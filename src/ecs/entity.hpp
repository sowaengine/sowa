#pragma once

#include <algorithm>
#include <cstddef>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

#include <flecs.h>

#include "core/reflection/field_ref.hpp"
#include "core/types.hpp"
#include "ecs/component_type.hpp"
#include "ecs/group.hpp"
#include "ecs/visibility.hpp"

namespace sowa {

// Entity handle with a generation counter: recycled IDs do not alias stale handles.
// Provides typed and reflected component access, naming, and hierarchy operations.
// Operations on null or dead entities return empty results or do nothing.
class Entity {
  public:
	// A default-constructed Entity is a null handle: not alive, not valid,
	// safe to destroy (no-op).
	Entity() = default;

	explicit Entity(flecs::entity entity) : _entity(entity) {
	}

	// Check ID first: flecs is_alive() aborts for a zero ID with a live world.
	bool IsAlive() const {
		return _entity.id() != 0 && _entity.is_alive();
	}

	// Immediately destroys the entity and its subtree. No-op for null or dead handles.
	// Unsafe inside Query::Each; use QueueDestroy during gameplay updates.
	void Destroy() {
		if (!IsAlive()) {
			return;
		}
		_entity.destruct();
		_entity = flecs::entity();
	}

	// Queues destruction after World::Progress. Safe during iteration; dead entities are ignored.
	void QueueDestroy();

	// --- Native-typed component ops ---

	// Ensures T is present, default-constructed. No-op if already present
	// (existing value is left alone) or if the entity is dead.
	template <typename T> void Add() {
		if (!IsAlive()) {
			return;
		}
		if (_entity.has<T>()) {
			return;
		}
		_entity.add<T>();
	}

	// Adds (or replaces) a native component value on this entity.
	template <typename T> void Add(const T &value) {
		if (!IsAlive()) {
			return;
		}
		_entity.set<T>(value);
	}

	template <typename T> bool Has() const {
		if (!IsAlive()) {
			return false;
		}
		return _entity.has<T>();
	}

	// Borrowed component storage; null if absent or dead. Structural changes can relocate
	// the table. Retrieve again after any such change.
	template <typename T> T *Get() {
		if (!IsAlive()) {
			return nullptr;
		}
		return _entity.try_get_mut<T>();
	}

	template <typename T> const T *Get() const {
		if (!IsAlive()) {
			return nullptr;
		}
		return _entity.try_get<T>();
	}

	template <typename T> void Remove() {
		if (!IsAlive()) {
			return;
		}
		_entity.remove<T>();
	}

	// --- Untyped / ComponentType ops (data-described components) ---

	// Ensures the component is present with default-initialized storage.
	// Idempotent: if already present, existing values are left alone.
	void Add(ComponentType type) {
		if (!IsAlive() || !type.IsValid()) {
			return;
		}
		const flecs::id_t id = type.Raw().id();
		if (_entity.has(id)) {
			return;
		}
		_entity.add(id);
	}

	bool Has(ComponentType type) const {
		if (!IsAlive() || !type.IsValid()) {
			return false;
		}
		return _entity.has(type.Raw().id());
	}

	// Mutable raw blob; nullptr if dead, type invalid, or not present.
	// Borrows live storage - same invalidation caveat as the typed Get<T>().
	// Field-level reads/writes go through GetField below.
	void *Get(ComponentType type) {
		if (!IsAlive() || !type.IsValid()) {
			return nullptr;
		}
		return _entity.try_get_mut(type.Raw().id());
	}

	const void *Get(ComponentType type) const {
		if (!IsAlive() || !type.IsValid()) {
			return nullptr;
		}
		return _entity.try_get(type.Raw().id());
	}

	void Remove(ComponentType type) {
		if (!IsAlive() || !type.IsValid()) {
			return;
		}
		_entity.remove(type.Raw().id());
	}

	// Field access.

	// Borrows a reflected field. Invalid for missing entities, types, components, or fields.
	// Structural changes invalidate the view.
	FieldRef GetField(ComponentType type, std::string_view name) {
		const FieldInfo *info = type.FindField(name);
		if (info == nullptr) {
			return FieldRef();
		}

		void *base = Get(type);
		if (base == nullptr) {
			return FieldRef();
		}

		return FieldRef(static_cast<u8 *>(base) + info->offset, info);
	}

	// Naming and path lookup. Names are scoped to the parent; paths use / separators.

	// Assigns a name and returns it. Appends a counter to avoid sibling collisions
	// ("Sprite", "Sprite2", ...). Replaces / and : with _. Empty clears the name;
	// assigning the current name is a no-op. Returns an empty string for a dead entity.
	std::string SetName(std::string_view name);

	// This entity's own name, or "" if unnamed or dead.
	std::string GetName() const {
		if (!IsAlive()) {
			return std::string();
		}
		const char *name = _entity.name();
		return name != nullptr ? std::string(name) : std::string();
	}

	// Full '/'-separated path from the root, e.g. "Player/Sprite". Empty for
	// a dead entity.
	std::string GetPath() const {
		if (!IsAlive()) {
			return std::string();
		}
		return std::string(_entity.path("/", ""));
	}

	// Resolves a /-separated path within this subtree. Returns null if absent; never searches
	// parents.
	Entity Find(std::string_view path) const;

	// Groups are cross-cutting labels represented by InGroup relationships.
	// World::EachInGroup visits their members across the world.

	// Adds this entity to name, creating the group if it doesn't exist.
	// Idempotent. No-op on a dead entity or an empty name.
	void AddGroup(std::string_view name);

	// Removes this entity from name. No-op if it isn't a member.
	void RemoveGroup(std::string_view name);

	bool IsInGroup(std::string_view name) const;

	// Every group this entity belongs to, for the inspector and scripts.
	// Order is unspecified.
	std::vector<std::string> GetGroups() const;

	// Visibility. Hiding an entity hides its entire subtree through query filtering.

	// Shows or hides this entity and its whole subtree. No-op if dead.
	void SetVisible(bool visible) {
		if (!IsAlive()) {
			return;
		}
		if (visible) {
			_entity.remove<Hidden>();
		} else {
			_entity.add<Hidden>();
		}
	}

	// Local visibility flag. Use IsVisibleInTree to include ancestor visibility.
	bool IsVisible() const {
		return IsAlive() && !_entity.has<Hidden>();
	}

	// Checks visibility through the ancestor chain in O(depth).
	// Use VisibilityFilter::VisibleOnly for rendering queries.
	bool IsVisibleInTree() const {
		if (!IsAlive()) {
			return false;
		}
		for (Entity current = *this; current.IsAlive(); current = current.GetParent()) {
			if (current._entity.has<Hidden>()) {
				return false;
			}
		}
		return true;
	}

	// Disabled entities are excluded from queries and systems by default. Hidden only
	// affects rendering. Editor snapshots are disabled so only the live scene runs.

	void SetEnabled(bool enabled) {
		if (!IsAlive()) {
			return;
		}
		if (enabled) {
			_entity.enable();
		} else {
			_entity.disable();
		}
	}

	// False for a dead handle.
	bool IsEnabled() const {
		return IsAlive() && _entity.enabled();
	}

	// Hierarchy uses flecs ChildOf for recursive destruction and parent-first traversal.
	// Children remain in insertion order for stable serialization and editor display.

	// Returns registered component types, excluding internal flecs types and relationships.
	// Order is unspecified.
	std::vector<ComponentType> GetComponents() const;

	// Creates a child, parenting before naming so uniqueness uses the destination scope.
	// Returns null if this entity is dead.
	Entity CreateChild(const std::string &name = "") {
		if (!IsAlive()) {
			return Entity();
		}

		Entity child(_entity.world().entity());
		child.SetParent(*this);
		if (!name.empty()) {
			child.SetName(name);
		}
		return child;
	}

	// Replaces the parent. Returns false without changes for dead handles, self-parenting,
	// or parenting into this entity's subtree; flecs does not enforce the acyclic trait.
	bool SetParent(Entity parent) {
		if (!IsAlive() || !parent.IsAlive()) {
			return false;
		}
		if (parent == *this || parent.IsDescendantOf(*this)) {
			return false;
		}

		// Enable OrderedChildren before adding the first child. Clear the name before moving
		// and assign a unique name afterwards to avoid corrupting the destination name index.
		const std::string name = takeName();
		parent._entity.add(flecs::OrderedChildren);
		_entity.child_of(parent._entity);
		if (!name.empty()) {
			SetName(name);
		}
		return true;
	}

	// Detaches from the current parent, making this a root entity. No-op if
	// dead or already a root.
	void ClearParent() {
		if (!IsAlive()) {
			return;
		}
		// Same clear-move-restore dance as SetParent: root scope can collide
		// too, with another root entity or with a registered component.
		const std::string name = takeName();
		_entity.remove(flecs::ChildOf, flecs::Wildcard);
		if (!name.empty()) {
			SetName(name);
		}
	}

	// The parent, or a null Entity if this is a root (or dead). Normalized
	// to a genuinely default-constructed handle rather than flecs' zero-id-
	// with-a-world, so GetParent() == Entity() holds for a root.
	Entity GetParent() const {
		if (!IsAlive()) {
			return Entity();
		}
		const flecs::entity parent = _entity.parent();
		if (parent.id() == 0) {
			return Entity();
		}
		return Entity(parent);
	}

	bool HasParent() const {
		return GetParent().IsAlive();
	}

	// Invokes func(Entity) for each direct child, in insertion order.
	// Don't reparent or destroy from inside the callback - collect first,
	// act after, the same rule Query::Each documents.
	template <typename Func> void EachChild(Func &&func) const {
		if (!IsAlive()) {
			return;
		}
		_entity.children([&](flecs::entity child) { func(Entity(child)); });
	}

	// Direct children in insertion order. Allocates - EachChild is the
	// cheaper path; this exists for callers that need a container (the
	// editor tree, reordering).
	std::vector<Entity> GetChildren() const {
		std::vector<Entity> children;
		EachChild([&](Entity child) { children.push_back(child); });
		return children;
	}

	// Whether ancestor is this entity's parent, grandparent, and so on.
	// False for an unrelated entity, for itself, and for dead handles.
	bool IsDescendantOf(Entity ancestor) const {
		if (!IsAlive() || !ancestor.IsAlive()) {
			return false;
		}
		for (Entity current = GetParent(); current.IsAlive(); current = current.GetParent()) {
			if (current == ancestor) {
				return true;
			}
		}
		return false;
	}

	// Reorders children. Rejects lists that do not contain exactly the current child set.
	bool SetChildOrder(const std::vector<Entity> &children) {
		if (!IsAlive()) {
			return false;
		}

		const std::vector<Entity> current = GetChildren();
		if (current.size() != children.size()) {
			return false;
		}
		for (const Entity &child : children) {
			if (!child.IsAlive() || child.GetParent() != *this) {
				return false;
			}
			if (std::count(children.begin(), children.end(), child) != 1) {
				return false;
			}
		}

		std::vector<flecs::entity_t> ids;
		ids.reserve(children.size());
		for (const Entity &child : children) {
			ids.push_back(child.Raw().id());
		}

		_entity.set_child_order(ids.data(), static_cast<i32>(ids.size()));
		return true;
	}

	// Duplication.

	// Copies the subtree, component values, groups, visibility, and child order. Internal
	// entity references map to copies; external references and ResourceIDs stay unchanged.
	// Null parent creates a root. Rejects dead handles, cycles, and deferred execution.
	Entity Duplicate(Entity parent = Entity()) const;

	bool operator==(const Entity &other) const {
		return _entity == other._entity;
	}

	bool operator!=(const Entity &other) const {
		return !(*this == other);
	}

	// Escape hatch for other src/ecs/ code that needs the underlying flecs
	// handle (e.g. World, or later component/query APIs). Not intended for
	// use outside src/ecs/.
	flecs::entity Raw() const {
		return _entity;
	}

  private:
	// Returns this entity's current name and clears it, so a scope change
	// can re-apply it (and re-uniquify) without a duplicate ever existing.
	std::string takeName();

	flecs::entity _entity;
};

// An entity-reference field stores a sowa::Entity, on both the native and
// the data-described path - see the FieldType docs for why one layout per
// field type matters. Specialized here rather than in field_type.hpp so that
// header stays free of a dependency on flecs.
SOWA_FIELD_TYPE_OF(Entity, FieldType::EntityRef);

} // namespace sowa

template <> struct std::hash<sowa::Entity> {
	size_t operator()(const sowa::Entity &entity) const noexcept {
		return std::hash<flecs::entity_t>{}(entity.Raw().id());
	}
};
