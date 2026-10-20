#include "ecs/entity.hpp"

#include <string>
#include <unordered_map>

#include "core/log.hpp"
#include "ecs/group.hpp"
#include "ecs/world.hpp"

namespace sowa {

std::vector<ComponentType> Entity::GetComponents() const {
	std::vector<ComponentType> components;
	if (!IsAlive()) {
		return components;
	}

	_entity.each([&](flecs::id id) {
		// Pairs are relationships (ChildOf, InGroup), not components.
		if (id.is_pair()) {
			return;
		}

		flecs::entity type = id.entity();
		// A FieldTable is what RegisterComponent leaves behind, so it's the
		// marker separating our components from flecs' own bookkeeping and
		// from types auto-registered by a stray Add<T>().
		if (type.has<FieldTable>()) {
			components.emplace_back(type);
		}
	});
	return components;
}

void Entity::QueueDestroy() {
	if (!IsAlive()) {
		return;
	}

	World *world = static_cast<World *>(_entity.world().get_ctx());
	if (world == nullptr) {
		Log::Error("Entity::QueueDestroy: entity has no World");
		return;
	}
	world->QueueDestroy(*this);
}

namespace {

// Public paths use / instead of the internal flecs separator.
constexpr const char *kPathSeparator = "/";

// Both would be parsed as path separators (flecs uses ':' internally), so a
// name containing them could never be looked up by the path it reports.
std::string SanitizeName(std::string_view name) {
	std::string sanitized(name);
	for (char &c : sanitized) {
		if (c == '/' || c == ':') {
			c = '_';
		}
	}
	return sanitized;
}

// Looks up name directly inside scope (0 = root), without searching
// parent scopes. Returns 0 if nothing there has that name.
flecs::entity_t LookupIn(const flecs::world_t *world, flecs::entity_t scope,
	const std::string &name) {
	return ecs_lookup_path_w_sep(world, scope, name.c_str(), kPathSeparator, kPathSeparator, false);
}

// The scope that owns every group entity, created on first use.
flecs::entity GroupScope(flecs::world_t *world) {
	return flecs::world(world).entity(kGroupScopeName);
}

// Resolves an existing group, or a null handle if no such group exists.
flecs::entity FindGroup(flecs::world_t *world, const std::string &name) {
	const flecs::entity_t scope = LookupIn(world, 0, std::string(kGroupScopeName));
	if (scope == 0) {
		return flecs::entity();
	}
	const flecs::entity_t group = LookupIn(world, scope, name);
	if (group == 0) {
		return flecs::entity();
	}
	return flecs::entity(world, group);
}

} // namespace

void Entity::AddGroup(std::string_view name) {
	if (!IsAlive() || name.empty()) {
		return;
	}

	const std::string sanitized = SanitizeName(name);
	flecs::world_t *world = _entity.world().c_ptr();

	const flecs::entity scope = GroupScope(world);
	flecs::entity group(world, LookupIn(world, scope.id(), sanitized));
	if (group.id() == 0) {
		// Set the scope before naming: world.entity(name) may resolve an existing root entity.
		group = flecs::world(world).entity();
		group.child_of(scope);
		group.set_name(sanitized.c_str());
	}

	// add() is idempotent, so joining twice is harmless.
	_entity.add<InGroup>(group);
}

void Entity::RemoveGroup(std::string_view name) {
	if (!IsAlive() || name.empty()) {
		return;
	}

	flecs::world_t *world = _entity.world().c_ptr();
	const flecs::entity group = FindGroup(world, SanitizeName(name));
	if (group.id() == 0) {
		return;
	}

	_entity.remove<InGroup>(group);
}

bool Entity::IsInGroup(std::string_view name) const {
	if (!IsAlive() || name.empty()) {
		return false;
	}

	flecs::world_t *world = _entity.world().c_ptr();
	const flecs::entity group = FindGroup(world, SanitizeName(name));
	if (group.id() == 0) {
		return false;
	}

	return _entity.has<InGroup>(group);
}

std::vector<std::string> Entity::GetGroups() const {
	std::vector<std::string> groups;
	if (!IsAlive()) {
		return groups;
	}

	_entity.each<InGroup>([&](flecs::entity group) {
		const char *name = group.name();
		if (name != nullptr) {
			groups.emplace_back(name);
		}
	});
	return groups;
}

std::string Entity::SetName(std::string_view name) {
	if (!IsAlive()) {
		return std::string();
	}

	const std::string desired = SanitizeName(name);
	if (desired.empty()) {
		_entity.set_name(nullptr);
		return std::string();
	}

	// Renaming to the current name must not bump the counter.
	if (GetName() == desired) {
		return desired;
	}

	// Uniqueness is per parent scope. At the root that scope also holds
	// registered components and systems, so this naturally avoids colliding
	// with them too.
	flecs::world_t *world = _entity.world().c_ptr();
	const flecs::entity_t scope = _entity.parent().id();

	std::string candidate = desired;
	for (u32 suffix = 2; suffix != 0; ++suffix) {
		const flecs::entity_t existing = LookupIn(world, scope, candidate);
		if (existing == 0 || existing == _entity.id()) {
			break;
		}
		candidate = desired + std::to_string(suffix);
	}

	_entity.set_name(candidate.c_str());
	return candidate;
}

std::string Entity::takeName() {
	if (!IsAlive()) {
		return std::string();
	}

	std::string current = GetName();
	if (!current.empty()) {
		_entity.set_name(nullptr);
	}
	return current;
}

namespace {

using EntityMap = std::unordered_map<Entity, Entity>;

// Copies one entity's components - not its children - and puts the copy in
// its final place.
Entity CloneOne(Entity source, Entity parent) {
	// flecs clones component values through copy hooks, including groups and visibility.
	// Assign the new name after parenting.
	Entity copy(source.Raw().clone(true));

	// The clone also carries (ChildOf, source's parent) across, so an
	// unparented duplicate has to be actively detached rather than just left
	// where it lands.
	if (parent.IsAlive()) {
		copy.SetParent(parent);
	} else {
		copy.ClearParent();
	}

	copy.SetName(source.GetName());
	return copy;
}

Entity CloneSubtree(Entity source, Entity parent, EntityMap &mapping) {
	Entity copy = CloneOne(source, parent);
	mapping.emplace(source, copy);

	// Copies land under copy, never under source, so this list can't grow
	// while it's being walked.
	for (Entity child : source.GetChildren()) {
		CloneSubtree(child, copy, mapping);
	}

	return copy;
}

// Remap references after cloning every entity so forward references find their copies.
void RemapEntityRefs(const EntityMap &mapping) {
	for (const auto &pair : mapping) {
		Entity copy = pair.second;
		for (ComponentType type : copy.GetComponents()) {
			for (const FieldInfo &field : type.Fields()) {
				if (field.type != FieldType::EntityRef) {
					continue;
				}

				Entity *target = copy.GetField(type, field.name).As<Entity>();
				if (target == nullptr) {
					continue;
				}

				const auto found = mapping.find(*target);
				// Anything outside the copied subtree stays as it was: it's a
				// reference to a thing that wasn't duplicated, so there is no
				// copy of it to point at.
				if (found != mapping.end()) {
					*target = found->second;
				}
			}
		}
	}
}

} // namespace

Entity Entity::Duplicate(Entity parent) const {
	if (!IsAlive()) {
		return Entity();
	}

	if (_entity.world().is_deferred()) {
		// flecs defers clones during iteration, leaving no component storage to remap yet.
		// Reject the operation rather than return a copy with references into the original.
		Log::Error("Entity::Duplicate cannot run while flecs is deferring "
				   "(inside a system or query iteration)");
		return Entity();
	}

	// Copying into the source subtree would recurse into newly created children.
	if (parent.IsAlive() && (parent == *this || parent.IsDescendantOf(*this))) {
		return Entity();
	}

	EntityMap mapping;
	Entity copy = CloneSubtree(*this, parent, mapping);
	RemapEntityRefs(mapping);
	return copy;
}

std::unordered_map<Entity, Entity> Entity::DuplicateRoots(const std::vector<Entity> &roots) {
	EntityMap mapping;
	if (roots.empty()) {
		return mapping;
	}
	for (Entity root : roots) {
		if (!root.IsAlive() || root.Raw().world().is_deferred() ||
			root.Raw().world().c_ptr() != roots.front().Raw().world().c_ptr()) {
			return {};
		}
		for (Entity other : roots) {
			if (root != other && root.IsDescendantOf(other)) {
				return {};
			}
		}
	}
	for (Entity root : roots) {
		if (!mapping.contains(root)) {
			CloneSubtree(root, root.GetParent(), mapping);
		}
	}
	RemapEntityRefs(mapping);
	return mapping;
}

Entity Entity::Find(std::string_view path) const {
	if (!IsAlive() || path.empty()) {
		return Entity();
	}

	flecs::world_t *world = _entity.world().c_ptr();
	const std::string query(path);
	const flecs::entity_t found = ecs_lookup_path_w_sep(world, _entity.id(), query.c_str(),
		kPathSeparator, kPathSeparator, false);

	if (found == 0) {
		return Entity();
	}
	return Entity(flecs::entity(world, found));
}

} // namespace sowa
