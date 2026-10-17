#include "resource/resource.hpp"

#include "core/reflection/field_serializer.hpp"

namespace sowa {

namespace {

const std::vector<FieldInfo> &emptyFields() {
	static const std::vector<FieldInfo> empty;
	return empty;
}

} // namespace

const std::vector<FieldInfo> &Resource::Fields() const {
	return _fields != nullptr ? *_fields : emptyFields();
}

void Resource::Serialize(Archive &archive) {
	// Use the shared reflected-field serializer. ResourceTypeBuilder rejects EntityRef,
	// so no ECS fallback is needed.
	SerializeFields(archive, this, Fields());
}

} // namespace sowa
