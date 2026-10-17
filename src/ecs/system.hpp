#pragma once

#include <cstddef>
#include <functional>

#include <flecs.h>

namespace sowa {

// Type-erased handle to a registered flecs system, supporting native and script-defined
// systems.
class System {
  public:
	// A default-constructed System is invalid; all operations are safe
	// no-ops on it.
	System() = default;

	explicit System(flecs::entity entity) : _entity(entity) {
	}

	bool IsValid() const {
		return _entity.is_valid() && _entity.is_alive();
	}

	// Disabled systems remain registered but are skipped by World::Progress.
	void SetEnabled(bool enabled) {
		if (!IsValid()) {
			return;
		}
		if (enabled) {
			_entity.enable();
		} else {
			_entity.disable();
		}
	}

	// False for an invalid handle.
	bool IsEnabled() const {
		return IsValid() && _entity.enabled();
	}

	// Unregisters the system entirely. No-op if already invalid.
	void Destroy() {
		if (!IsValid()) {
			return;
		}
		_entity.destruct();
		_entity = flecs::entity();
	}

	bool operator==(const System &other) const {
		return _entity == other._entity;
	}

	bool operator!=(const System &other) const {
		return !(*this == other);
	}

	// Escape hatch for other src/ecs/ code. Not intended for use outside it.
	flecs::entity Raw() const {
		return _entity;
	}

  private:
	flecs::entity _entity;
};

} // namespace sowa

template <> struct std::hash<sowa::System> {
	size_t operator()(const sowa::System &system) const noexcept {
		return std::hash<flecs::entity_t>{}(system.Raw().id());
	}
};
