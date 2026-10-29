#pragma once

#include "core/reflection/field_serializer.hpp"
#include "core/serialize/archive.hpp"
#include "ecs/component_type.hpp"
#include "ecs/entity.hpp"

namespace sowa {

// Transfers serialized fields for native or runtime components. Loading adds missing
// components and preserves absent fields. Saving fails for absent components.
// EntityRef loads require a fallback to collect paths for later resolution.
bool SerializeComponent(Archive &archive, Entity entity, ComponentType type,
	FieldFallback fallback = nullptr, void *context = nullptr,
	ResourceIdMapper resourceMapper = nullptr, void *resourceContext = nullptr);

} // namespace sowa
