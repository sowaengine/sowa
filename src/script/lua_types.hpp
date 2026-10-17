#pragma once

#include <string>

#include "ecs/component_type.hpp"
#include "ecs/entity.hpp"
#include "resource/resource_id.hpp"

namespace sowa {

class ResourceRegistry;

// Resolves storage on each access because component changes can move the entity.
struct ComponentView {
	Entity entity;
	ComponentType type;
};

// Writes vector/color fields through Entity::GetField.
// After component removal, reads return zero and writes do nothing.
struct FieldProxy {
	Entity entity;
	ComponentType type;
	std::string fieldName;
};

// Resource view resolved by ID on every field access. Lua does not retain resources.
// The registry must outlive the Lua state.
struct ResourceView {
	ResourceRegistry *registry = nullptr;
	ResourceID id;
};

// Writes vector/color fields through ResourceRegistry::Get.
// After resource removal, reads return zero and writes do nothing.
struct ResourceFieldProxy {
	ResourceRegistry *registry = nullptr;
	ResourceID id;
	std::string fieldName;
};

} // namespace sowa
