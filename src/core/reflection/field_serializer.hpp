#pragma once

#include <vector>

#include "core/reflection/field_ref.hpp"
#include "core/reflection/field_type.hpp"
#include "core/serialize/archive.hpp"

namespace sowa {

// Handles fields that core cannot transfer, currently EntityRef. The context lets
// scene loading collect references for resolution after all entities exist.
using FieldFallback = void (*)(void *context, Archive &, const FieldInfo &, FieldRef);

// Transfers reflected fields between an object and an Archive using base + offset.
// Works for ECS components and resources. Skips fields without FieldFlags::Serialize;
// the caller supplies nesting.
void SerializeFields(Archive &archive, void *base, const std::vector<FieldInfo> &fields,
	FieldFallback fallback = nullptr, void *context = nullptr);

} // namespace sowa
