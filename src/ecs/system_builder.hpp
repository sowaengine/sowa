#pragma once

#include <string>
#include <utility>
#include <vector>

#include <flecs.h>

#include "ecs/component_type.hpp"
#include "ecs/entity.hpp"
#include "ecs/system.hpp"
#include "ecs/visibility.hpp"

namespace sowa {

// Builds a system through World::CreateSystem<Components...>(name); Each(func)
// registers it. Without<T>() excludes matching tables. Systems are play-only by
// default; Always() keeps them running while the application is stopped.
template <typename... Components> class SystemBuilder {
  public:
	SystemBuilder(flecs::world &world, const std::string &name,
		std::vector<System> *playOnlySystems = nullptr,
		const bool *playOnlySystemsEnabled = nullptr)
		: _builder(world.template system<Components...>(name.c_str())),
		  _playOnlySystems(playOnlySystems), _playOnlySystemsEnabled(playOnlySystemsEnabled) {
	}

	// Excludes entities that have T. Chainable.
	template <typename T> SystemBuilder &Without() {
		_builder.template without<T>();
		return *this;
	}

	// Excludes entities that have a runtime-registered component. The
	// ComponentType overload is what a script-defined filter would use; it's
	// here because it costs nothing over the typed one.
	SystemBuilder &Without(ComponentType type) {
		if (type.IsValid()) {
			_builder.without(type.Raw().id());
		}
		return *this;
	}

	// Excludes hidden entities and their descendants at the table level.
	SystemBuilder &VisibleOnly() {
		ApplyVisibleOnly(_builder);
		return *this;
	}

	// Runs even while play-only systems are off - preview/draw-style work
	// (particles, sprite animation), not gameplay (bullets, spawners).
	// RegisterSystem and CreateSystem default to play-only.
	SystemBuilder &Always() {
		_always = true;
		return *this;
	}

	// Registers the system with func(Entity, Components&...) as its body.
	System Each(auto &&func) {
		flecs::system system = _builder.each(
			[callback = std::forward<decltype(func)>(func)](flecs::entity entity,
				Components &...components) { callback(Entity(entity), components...); });
		System handle(system);
		if (!_always && _playOnlySystems != nullptr) {
			_playOnlySystems->push_back(handle);
			if (_playOnlySystemsEnabled != nullptr && !*_playOnlySystemsEnabled) {
				handle.SetEnabled(false);
			}
		}
		return handle;
	}

  private:
	flecs::system_builder<Components...> _builder;
	std::vector<System> *_playOnlySystems = nullptr;
	const bool *_playOnlySystemsEnabled = nullptr;
	bool _always = false;
};

} // namespace sowa
