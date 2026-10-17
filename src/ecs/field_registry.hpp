#pragma once

#include <vector>

#include <flecs.h>

#include "core/reflection/field_type.hpp"
#include "math/color.hpp"
#include "math/vector2.hpp"
#include "math/vector3.hpp"

namespace sowa {

// Internal field registration shared by native and runtime-defined components.
// Both paths use the same FieldType-to-flecs mapping to keep storage layouts consistent.

// Maps reflected types to flecs storage. String uses std::string; EntityRef uses Entity
// on both registration paths. Vector and color types are opaque leaves. Enums require
// constants.
flecs::entity_t ResolveFieldTypeId(flecs::world &world, const FieldDesc &desc);

// Finds or creates the named flecs enum. Constants are child entities with an
// (EcsConstant, i32) pair, avoiding ecs_enum_init's fixed-size descriptor array.
// An existing enum is reused with its constants unchanged.
flecs::entity_t ResolveEnumTypeId(flecs::world &world, const std::string &name,
	const std::vector<EnumConstant> &constants);

// Adds a struct member. Native registration supplies an explicit offset;
// runtime-described layouts let flecs calculate offsets.
void AddComponentField(flecs::world &world, flecs::entity component, const FieldDesc &desc,
	size_t offset, bool useOffset);

// Caches field descriptions with their resolved flecs offsets.
// Descriptions must match member registration order.
void RefreshFieldTable(flecs::world &world, flecs::entity component,
	const std::vector<FieldDesc> &descs);

} // namespace sowa
