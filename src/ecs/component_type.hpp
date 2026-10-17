#pragma once

#include <cstddef>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

#include <flecs.h>

#include "core/reflection/field_type.hpp"

namespace sowa {

// Reflection table stored on the flecs component entity and freed with it.
// RefreshFieldTable populates it; use ComponentType::Fields()/FindField() to read it.
struct FieldTable {
	std::vector<FieldInfo> fields;
};

// Handle to a registered component type, returned by World::RegisterComponent.
// Wraps the flecs entity carrying layout and reflection metadata.
class ComponentType {
  public:
	// A default-constructed ComponentType is invalid/unregistered.
	ComponentType() = default;

	explicit ComponentType(flecs::entity entity) : _entity(entity) {
	}

	bool IsValid() const {
		return _entity.is_valid();
	}

	// Registered name used in scene files and by World::FindComponent. Empty for an invalid
	// handle.
	std::string Name() const {
		if (!IsValid()) {
			return {};
		}
		const char *name = _entity.name();
		return name != nullptr ? std::string(name) : std::string();
	}

	// Reflected fields in registration order, including internal fields. The reference
	// lasts until the component type or field table changes.
	const std::vector<FieldInfo> &Fields() const {
		static const std::vector<FieldInfo> empty;
		if (!IsValid()) {
			return empty;
		}
		const FieldTable *table = _entity.try_get<FieldTable>();
		return table != nullptr ? table->fields : empty;
	}

	// Looks up one field by name; nullptr if this type has no such field.
	const FieldInfo *FindField(std::string_view name) const {
		for (const FieldInfo &field : Fields()) {
			if (field.name == name) {
				return &field;
			}
		}
		return nullptr;
	}

	bool operator==(const ComponentType &other) const {
		return _entity == other._entity;
	}

	bool operator!=(const ComponentType &other) const {
		return !(*this == other);
	}

	// Escape hatch for other src/ecs/ code (World, later component/query
	// APIs) that needs the underlying flecs handle. Not intended for use
	// outside src/ecs/.
	flecs::entity Raw() const {
		return _entity;
	}

  private:
	flecs::entity _entity;
};

} // namespace sowa

template <> struct std::hash<sowa::ComponentType> {
	size_t operator()(const sowa::ComponentType &type) const noexcept {
		return std::hash<flecs::entity_t>{}(type.Raw().id());
	}
};
