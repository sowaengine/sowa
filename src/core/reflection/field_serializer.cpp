#include "core/reflection/field_serializer.hpp"

#include <string>

namespace sowa {

void SerializeFields(Archive &archive, void *base, const std::vector<FieldInfo> &fields,
	FieldFallback fallback, void *context) {
	if (!archive.IsValid() || base == nullptr) {
		return;
	}

	for (const FieldInfo &field : fields) {
		if (!field.IsSerialized()) {
			continue;
		}

		FieldRef ref(static_cast<unsigned char *>(base) + field.offset, &field);

		// As<T>() can't fail here - the switch already matched the field's own
		// type - but it's checked anyway rather than dereferenced blind, since
		// the cost is nothing and the failure mode would be a wild write.
		switch (field.type) {
		case FieldType::Bool:
			if (bool *value = ref.As<bool>()) {
				archive.Field(field.name, *value);
			}
			break;
		case FieldType::I32:
			if (i32 *value = ref.As<i32>()) {
				archive.Field(field.name, *value);
			}
			break;
		case FieldType::U32:
			if (u32 *value = ref.As<u32>()) {
				archive.Field(field.name, *value);
			}
			break;
		case FieldType::F32:
			if (f32 *value = ref.As<f32>()) {
				archive.Field(field.name, *value);
			}
			break;
		case FieldType::F64:
			if (f64 *value = ref.As<f64>()) {
				archive.Field(field.name, *value);
			}
			break;
		case FieldType::String:
			if (std::string *value = ref.As<std::string>()) {
				archive.Field(field.name, *value);
			}
			break;
		case FieldType::Vector2:
			if (Vector2 *value = ref.As<Vector2>()) {
				archive.Field(field.name, *value);
			}
			break;
		case FieldType::Vector3:
			if (Vector3 *value = ref.As<Vector3>()) {
				archive.Field(field.name, *value);
			}
			break;
		case FieldType::Color:
			if (Color *value = ref.As<Color>()) {
				archive.Field(field.name, *value);
			}
			break;
		case FieldType::Enum:
			// Reached through Data() rather than As<T>(): every reflected enum
			// stores as i32, and the enum's identity is already in the field's
			// own constant list, which is what the archive needs anyway.
			archive.EnumField(field.name, *static_cast<i32 *>(ref.Data()), field.enumConstants);
			break;
		case FieldType::ResourceRef:
			// Serialize the underlying u64 through Data(): core cannot depend on ResourceID.
			// Restored resource records and component references must retain the same ID.
			archive.Field(field.name, *static_cast<u64 *>(ref.Data()));
			break;
		case FieldType::EntityRef:
			// Only components can mean anything by this. A resource outlives
			// the scene whose entities it would be naming, so reaching here
			// without a fallback is a description bug, not a data problem.
			if (fallback != nullptr) {
				fallback(context, archive, field, ref);
			} else {
				archive.AddError(field.name, "entity references are only meaningful on components");
			}
			break;
		}
	}
}

} // namespace sowa
