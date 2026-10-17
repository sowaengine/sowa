#include "ecs/component_serializer.hpp"
#include "ecs/components/tilemap.hpp"

#include <string>

#include "core/reflection/field_ref.hpp"
#include "core/reflection/field_serializer.hpp"
#include "core/reflection/field_type.hpp"
#include "ecs/field_registry.hpp" // IWYU pragma: keep - FieldTypeOf<Entity>

namespace sowa {

namespace {

// Save entity references as paths. Scene loading resolves them after creating all
// entities because a reference may point to an entity not yet loaded.
// SerializeFields delegates this ECS-specific field type through its fallback.
void transferEntityRef(void *, Archive &archive, const FieldInfo &field, FieldRef ref) {
	Entity *target = ref.As<Entity>();
	if (target == nullptr) {
		return;
	}

	if (archive.IsSaving()) {
		std::string path = target->IsAlive() ? target->GetPath() : std::string();
		archive.Field(field.name, path);
		return;
	}

	std::string path;
	if (archive.Field(field.name, path) && !path.empty()) {
		archive.AddError(field.name, "entity references are resolved by the scene loader, "
									 "not by component deserialization");
	}
}

} // namespace

bool SerializeComponent(Archive &archive, Entity entity, ComponentType type, FieldFallback fallback,
	void *context) {
	if (!archive.IsValid() || !entity.IsAlive() || !type.IsValid()) {
		return false;
	}

	if (archive.IsLoading()) {
		entity.Add(type);
	} else if (!entity.Has(type)) {
		return false;
	}

	if (type.Name() == "Tilemap") {
		entity.Get<Tilemap>()->Serialize(archive);
		return true;
	}

	// Tags such as Hidden have no storage. Return before asking flecs for a zero-sized component.
	if (type.Fields().empty()) {
		return true;
	}

	// Storage stays valid during this non-structural field walk.
	void *base = entity.Get(type);
	if (base == nullptr) {
		return false;
	}

	SerializeFields(archive, base, type.Fields(),
		fallback != nullptr ? fallback : &transferEntityRef, context);
	return true;
}

} // namespace sowa
