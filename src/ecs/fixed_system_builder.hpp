#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <flecs.h>

#include "ecs/component_type.hpp"
#include "ecs/entity.hpp"
#include "ecs/system.hpp"
#include "ecs/visibility.hpp"

namespace sowa {

// New instances join the captured membership next tick.
struct FixedTickState {
	bool cancelled = false;
	std::unordered_map<flecs::entity_t, std::unordered_set<flecs::entity_t>> matches;
	bool Allows(flecs::iter &it, size_t row) const {
		auto found = matches.find(it.system().id());
		return !cancelled && found != matches.end() && found->second.contains(it.entity(row).id());
	}
	void RunGlobal(flecs::iter &it, const auto &callback) const {
		const auto system = it.system().id();
		const f32 delta = it.delta_time();
		// Flecs finalizes termless task iterators itself. Filter-only queries
		// allocate an iterator that this non-iterating callback must release.
		const auto *query = ecs_system_get(it.world().c_ptr(), system)->query;
		if (!(query->flags & EcsQueryMatchNothing)) {
			it.fini();
		}
		if (!cancelled && matches.contains(system)) {
			callback(Entity(), delta);
		}
	}
};

// Driven by PhysicsWorld outside the frame pipeline.
template <typename... Components>
class FixedSystemBuilder {
  public:
	FixedSystemBuilder(flecs::world &world, const std::string &name, std::vector<System> &systems,
					   const FixedTickState &tick)
		: _builder(world.template system<Components...>(name.c_str())), _systems(systems),
		  _tick(tick) {
		_builder.kind(0);
	}

	template <typename T>
	FixedSystemBuilder &Without() {
		_builder.template without<T>();
		return *this;
	}

	FixedSystemBuilder &Without(ComponentType type) {
		if (type.IsValid()) {
			_builder.without(type.Raw());
		}
		return *this;
	}

	FixedSystemBuilder &VisibleOnly() {
		ApplyVisibleOnly(_builder);
		return *this;
	}

	// func(Entity, f32 fixedDelta, Components&...)
	System Each(auto &&func) {
		flecs::system system;
		if constexpr (sizeof...(Components) == 0) {
			system = _builder.run([callback = std::forward<decltype(func)>(func), tick = &_tick](
									  flecs::iter &it) {
				tick->RunGlobal(it, callback);
			});
		} else {
			system = _builder.each([callback = std::forward<decltype(func)>(func), tick = &_tick](
									   flecs::iter &it, size_t row, Components &...components) {
				if (tick->Allows(it, row)) {
					callback(Entity(it.entity(row)), static_cast<f32>(it.delta_time()), components...);
				}
			});
		}
		System handle(system);
		_systems.push_back(handle);
		return handle;
	}

  private:
	flecs::system_builder<Components...> _builder;
	std::vector<System> &_systems;
	const FixedTickState &_tick;
};

} // namespace sowa
