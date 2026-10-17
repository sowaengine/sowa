#pragma once

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <flecs.h>

#include "core/reflection/field_type.hpp"
#include "core/types.hpp"
#include "ecs/component_builder.hpp"
#include "ecs/component_type.hpp"
#include "ecs/ecs_types.hpp"
#include "ecs/entity.hpp"
#include "ecs/fixed_system_builder.hpp"
#include "ecs/group.hpp"
#include "ecs/query.hpp"
#include "ecs/system.hpp"
#include "ecs/system_builder.hpp"
#include "ecs/visibility.hpp"

namespace sowa {

// Owns the flecs world and exposes entities, components, queries, systems, and frame
// progression.
class World {
  public:
	World();
	~World();

	World(const World &) = delete;
	World &operator=(const World &) = delete;
	// Not movable: flecs world context stores this so Entity::QueueDestroy
	// can find the destroy queue.
	World(World &&) = delete;
	World &operator=(World &&) = delete;

	// Runs enabled systems, then queued destruction. Zero delta runs systems without
	// advancing time, overriding flecs wall-clock behavior.
	void Progress(f32 deltaTime);

	// Destroys the entity at the end of the next Progress. It stays alive and queryable
	// until then. No-op for dead or already queued handles. Parent destruction includes children.
	void QueueDestroy(Entity entity);

	// Delta passed to the current Progress call.
	f32 DeltaTime() const;

	// Whether flecs is currently queueing structural changes (inside a
	// system). PackedScene::Instantiate uses this to create the tree now so
	// the caller can edit it, then SetParent which stays queued.
	bool IsDeferred() const;

	// Temporarily allow immediate component writes without flushing queued work.
	// Pair with ResumeDefer; not re-entrant. Parenting remains deferred after resuming.
	void SuspendDefer();
	void ResumeDefer();

	// Registers a play-only OnUpdate system in registration order. Structural changes
	// are deferred until it finishes. Use CreateSystem(...).Always() to run while stopped.
	template <typename... Components, typename Func>
	System RegisterSystem(const std::string &name, Func &&func) {
		flecs::system system =
			_world.template system<Components...>(name.c_str())
				.each([callback = std::forward<Func>(func)](flecs::entity entity,
						  Components &...components) { callback(Entity(entity), components...); });
		System handle(system);
		_playOnlySystems.push_back(handle);
		if (!_playOnlySystemsEnabled) {
			handle.SetEnabled(false);
		}
		return handle;
	}

	// Builds a filtered system; Each registers it.
	// Systems run during play unless Always is selected.
	template <typename... Components>
	SystemBuilder<Components...> CreateSystem(const std::string &name) {
		return SystemBuilder<Components...>(_world, name, &_playOnlySystems,
			&_playOnlySystemsEnabled);
	}

	// Fixed callbacks receive seconds explicitly; DeltaTime remains frame time.
	template <typename... Components>
	FixedSystemBuilder<Components...> CreateFixedSystem(const std::string &name) {
		return FixedSystemBuilder<Components...>(_world, name, _fixedSystems, _fixedTick);
	}

	System RegisterUntypedFixedSystem(const std::string &name,
		const std::vector<ComponentType> &with, const std::vector<ComponentType> &without,
		std::function<void(Entity, f32)> callback);

	// Structural edits commit after the tick; returns false on pause or cancellation.
	bool RunFixedTick(f32 delta, const std::function<void()> &prepare,
		const std::function<void()> &simulate);
	void RefreshTransforms();
	void CancelFixedTicks() { _fixedTick.cancelled = true; }
	bool FixedTicksCancelled() const { return _fixedTick.cancelled; }

	// Enables play-only systems. Always systems and engine transform/order passes remain
	// active. Enabled by default.
	void SetPlayOnlySystemsEnabled(bool enabled);

	bool PlayOnlySystemsEnabled() const {
		return _playOnlySystemsEnabled;
	}

	// Creates an empty entity. Naming and component attachment are separate operations.
	Entity CreateEntity();

	// Registers native storage and returns a reflection builder.
	// Duplicate names return EcsError::DuplicateName. Type()/Result() expose the result.
	template <typename T> ComponentBuilder<T> RegisterComponent(const std::string &name) {
		if (_componentsByName.find(name) != _componentsByName.end()) {
			return ComponentBuilder<T>(_world, ComponentType(), EcsError::DuplicateName);
		}

		ComponentType type{flecs::entity(_world.component<T>(name.c_str()))};
		// An empty FieldTable marks registered tags for GetComponents without exposing flecs
		// bookkeeping.
		RefreshFieldTable(_world, type.Raw(), {});
		_componentsByName.emplace(name, type);
		return ComponentBuilder<T>(_world, type, EcsError::Ok);
	}

	// Registers a runtime layout without a C++ struct. Rejects duplicate names.
	EcsResult<ComponentType> RegisterComponent(const std::string &name,
		const std::vector<FieldDesc> &fields);

	// Looks up a previously registered component by name. Returns an invalid
	// ComponentType if no component with that name has been registered.
	ComponentType FindComponent(const std::string &name) const;
	std::vector<ComponentType> Components() const;

	bool IsDataDescribed(ComponentType type) const;

	// Same names, FieldTypes, flags and enum identity. Order is ignored: a
	// Lua table's pair iteration is not stable, and a reload of the same
	// fields must not rebuild storage.
	bool ComponentFieldsMatch(ComponentType type, const std::vector<FieldDesc> &fields) const;

	// Every entity that currently has type. Disabled holders are included
	// when requested so a play-mode snapshot copy migrates with the live tree.
	template <typename Func>
	void EachWithType(ComponentType type, bool includeDisabled, Func &&func) {
		if (!type.IsValid()) {
			return;
		}
		auto builder = _world.query_builder().with(type.Raw());
		if (includeDisabled) {
			builder.query_flags(EcsQueryMatchDisabled);
		}
		builder.build().each([&](flecs::entity entity) { func(Entity(entity)); });
	}

	// Recreates runtime layouts, preserving fields with matching names and types. New
	// fields get defaults; removed fields are dropped. Rejects native or unknown types
	// and calls inside Progress.
	EcsResult<ComponentType> ReplaceDataDescribedFields(ComponentType type,
		const std::vector<FieldDesc> &fields);

	// Registers a system using runtime component handles.
	// An empty with-list runs once per frame with a null Entity; always enables edit-mode
	// execution.
	System RegisterUntypedSystem(const std::string &name, const std::vector<ComponentType> &with,
		const std::vector<ComponentType> &without, bool always,
		std::function<void(Entity)> callback);

	// Destroys system and drops it from the play-only list. No-op if invalid.
	void DestroyTrackedSystem(System system);

	// Adds an already-registered system to the play-only list (Lua _start /
	// _update). Always() systems are not tracked.
	void TrackPlayOnly(System system);

	// Escape hatch for ScriptServer observers and pair terms. Not game code.
	flecs::world &Flecs() {
		return _world;
	}

	const flecs::world &Flecs() const {
		return _world;
	}

	// Resolves a /-separated path from the root. Returns null if no entity matches.
	// Excludes component and system entities; use FindComponent for component types.
	// Entity::Find resolves relative paths.
	Entity Find(const std::string &path) const;

	// Groups.

	// Visits group members using a temporary query. No-op for unknown groups.
	template <typename Func> void EachInGroup(const std::string &name, Func &&func) {
		const Entity group = findGroupEntity(name);
		if (!group.IsAlive()) {
			return;
		}

		_world.query_builder().with<InGroup>(group.Raw()).build().each([&](flecs::entity entity) {
			func(Entity(entity));
		});
	}

	// Every group that currently exists, for the editor's group list.
	// Order is unspecified.
	std::vector<std::string> GetGroups() const;

	// Creates a cached query to reuse across frames. VisibleOnly excludes hidden entities
	// and descendants at the table level. See Query for iteration and lifetime rules.
	template <typename... Components>
	Query<Components...> CreateQuery(VisibilityFilter filter = VisibilityFilter::All) {
		auto builder = _world.template query_builder<Components...>();
		if (filter == VisibilityFilter::VisibleOnly) {
			ApplyVisibleOnly(builder);
		}
		return Query<Components...>(builder.cached().build());
	}

	// Visits entities with T, including tags without storage. Builds a temporary query.
	template <typename T, typename Func> void EachWith(Func &&func) {
		_world.query_builder().template with<T>().build().each(
			[&](flecs::entity entity) { func(Entity(entity)); });
	}

	// One-off iteration using a temporary query; excludes disabled entities. Reuse
	// CreateQuery for per-frame work or EachIncludingDisabled to include disabled entities.
	template <typename... Components, typename Func> void Each(Func &&func) {
		_world.template query<Components...>().each(
			[&](flecs::entity e, Components &...comps) { func(Entity(e), comps...); });
	}

	// One-off iteration including disabled entities.
	template <typename... Components, typename Func> void EachIncludingDisabled(Func &&func) {
		_world.template query_builder<Components...>()
			.query_flags(EcsQueryMatchDisabled)
			.build()
			.each([&](flecs::entity e, Components &...comps) { func(Entity(e), comps...); });
	}

  private:
	// Registers the components and systems the engine itself owns (currently
	// Transform2D and the CalculateTransforms pass). Called from the ctor, so
	// they exist before any caller can register anything.
	void registerBuiltins();

	// The group entity for name, or a null Entity if no such group exists.
	Entity findGroupEntity(const std::string &name) const;

	void flushDestroyed();

	struct ImmediateMutate;

	flecs::world _world;
	std::unordered_map<std::string, ComponentType> _componentsByName;
	std::vector<System> _playOnlySystems;
	std::vector<System> _fixedSystems;
	System _transformSystem;
	FixedTickState _fixedTick;
	bool _playOnlySystemsEnabled = true;
	std::vector<Entity> _destroyQueue;
	std::unique_ptr<ImmediateMutate> _immediate;
};

} // namespace sowa
