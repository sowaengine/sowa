#include "ecs/field_registry.hpp"

#include "resource/resource_id.hpp"

#include <string>

#include "ecs/component_type.hpp"
#include "ecs/entity.hpp"

namespace sowa {

flecs::entity_t ResolveEnumTypeId(flecs::world &world, const std::string &name,
	const std::vector<EnumConstant> &constants) {
	if (name.empty()) {
		return 0;
	}

	flecs::world_t *raw = world.c_ptr();

	// Create unnamed before assigning a name: world.entity(name) could reuse an unrelated entity.
	flecs::entity_t existing = ecs_lookup_path_w_sep(raw, 0, name.c_str(), "::", "::", false);
	if (existing != 0) {
		return existing;
	}

	flecs::entity type = world.entity();
	type.set_name(name.c_str());

	// Register i32 enum constants as scoped children for flecs reflection.
	EcsEnum *enumData =
		static_cast<EcsEnum *>(ecs_ensure_id(raw, type.id(), ecs_id(EcsEnum), sizeof(EcsEnum)));
	enumData->underlying_type = ecs_id(ecs_i32_t);
	ecs_modified_id(raw, type.id(), ecs_id(EcsEnum));

	const ecs_entity_t previousScope = ecs_set_scope(raw, type.id());
	for (const EnumConstant &constant : constants) {
		flecs::entity value = world.entity();
		value.set_name(constant.name.c_str());

		// Write the constant pair explicitly so zero is preserved as a value.
		const ecs_id_t pair = ecs_pair(EcsConstant, ecs_id(ecs_i32_t));
		void *storage = ecs_ensure_id(raw, value.id(), pair, sizeof(i32));
		*static_cast<i32 *>(storage) = constant.value;
		ecs_modified_id(raw, value.id(), pair);
	}
	ecs_set_scope(raw, previousScope);

	return type.id();
}

flecs::entity_t ResolveFieldTypeId(flecs::world &world, const FieldDesc &desc) {
	const FieldType type = desc.type;
	switch (type) {
	case FieldType::Bool:
		return ecs_id(ecs_bool_t);
	case FieldType::I32:
		return ecs_id(ecs_i32_t);
	case FieldType::U32:
		return ecs_id(ecs_u32_t);
	case FieldType::F32:
		return ecs_id(ecs_f32_t);
	case FieldType::F64:
		return ecs_id(ecs_f64_t);
	case FieldType::String: {
		flecs::component<std::string> component = world.component<std::string>("String");
		if (!component.has(ecs_id(EcsOpaque))) {
			// Tell flecs how to read/write a std::string so its own
			// serializers keep working, even though sowa's reflection
			// reaches the member directly through its offset.
			component.opaque(flecs::String)
				.serialize([](const flecs::serializer *serializer, const std::string *data) {
					const char *value = data->c_str();
					return serializer->value(flecs::String, &value);
				})
				.assign_string([](std::string *data, const char *value) {
					*data = value != nullptr ? value : "";
				});
		}
		return component.id();
	}
	case FieldType::EntityRef:
		return world.component<Entity>("EntityRef").id();
	case FieldType::ResourceRef:
		// Same treatment as EntityRef: a named component whose layout is the
		// sowa type itself, so the data-described path and the native path
		// agree on the storage - the one-FieldType-one-layout invariant.
		return world.component<ResourceID>("ResourceRef").id();
	case FieldType::Vector2:
		return world.component<Vector2>("Vector2").id();
	case FieldType::Vector3:
		return world.component<Vector3>("Vector3").id();
	case FieldType::Color:
		return world.component<Color>("Color").id();
	case FieldType::Enum:
		return ResolveEnumTypeId(world, desc.enumName, desc.enumConstants);
	}
	return 0;
}

void AddComponentField(flecs::world &world, flecs::entity component, const FieldDesc &desc,
	size_t offset, bool useOffset) {
	ecs_member_t member{};
	member.name = desc.name.c_str();
	member.type = ResolveFieldTypeId(world, desc);
	member.offset = static_cast<i32>(offset);
	member.use_offset = useOffset;
	ecs_struct_add_member(world.c_ptr(), component.id(), &member);
}

void RefreshFieldTable(flecs::world &world, flecs::entity component,
	const std::vector<FieldDesc> &descs) {
	(void)world;

	FieldTable table;
	table.fields.reserve(descs.size());

	// Use resolved flecs offsets for both native and runtime-described layouts.
	const EcsStruct *layout = component.try_get<EcsStruct>();
	const i32 memberCount = layout != nullptr ? ecs_vec_count(&layout->members) : 0;
	const ecs_member_t *members =
		layout != nullptr ? static_cast<const ecs_member_t *>(ecs_vec_first(&layout->members))
						  : nullptr;

	for (size_t i = 0; i < descs.size(); ++i) {
		FieldInfo info;
		info.name = descs[i].name;
		info.type = descs[i].type;
		info.flags = descs[i].flags;
		info.resourceType = descs[i].resourceType;
		info.enumName = descs[i].enumName;
		info.enumConstants = descs[i].enumConstants;
		info.offset =
			i < static_cast<size_t>(memberCount) ? static_cast<size_t>(members[i].offset) : 0;
		table.fields.push_back(std::move(info));
	}

	component.set<FieldTable>(std::move(table));
}

} // namespace sowa
