#pragma once

#include "core/reflection/field_type.hpp"
#include "core/types.hpp"

namespace sowa {

// Borrowed view of an entity component field, returned by Entity::GetField.
// As<T>() and Set<T>() check the reflected type before accessing storage.
// Adding or removing components can move that storage; do not retain across structural changes.
class FieldRef {
  public:
	// A default-constructed FieldRef is invalid: no data, no info.
	FieldRef() = default;

	FieldRef(void *data, const FieldInfo *info) : _data(data), _info(info) {
	}

	bool IsValid() const {
		return _data != nullptr && _info != nullptr;
	}

	explicit operator bool() const {
		return IsValid();
	}

	// Only meaningful when IsValid(); returns F32 on an invalid ref.
	FieldType Type() const {
		return _info != nullptr ? _info->type : FieldType::F32;
	}

	// Reflection info for this field (name/type/flags/offset), or nullptr.
	const FieldInfo *Info() const {
		return _info;
	}

	// Raw storage pointer. For consumers that dispatch on Type() themselves.
	void *Data() const {
		return _data;
	}

	// Typed pointer into the field's storage, or nullptr if the ref is
	// invalid or T isn't the field's type. Writing through it is fine - it
	// points at the live component.
	template <typename T> T *As() const {
		static_assert(FieldTypeOf<T>::Valid, "T is not a supported field type (see FieldType)");
		if (!IsValid() || _info->type != FieldTypeOf<T>::Value) {
			return nullptr;
		}

		// Every reflected enum stores as i32, so matching on FieldType alone
		// would let a ZIndexMode* be handed out for a RenderLayerMode field.
		// The enum's identity is the second half of the check.
		if constexpr (FieldTypeOf<T>::Value == FieldType::Enum) {
			static_assert(EnumTraits<T>::Valid, "enum has no SOWA_REFLECT_ENUM declaration");
			if (_info->enumName != EnumTraits<T>::Name()) {
				return nullptr;
			}
		}
		return static_cast<T *>(_data);
	}

	// Returns false (leaving out untouched) on an invalid ref or type
	// mismatch.
	template <typename T> bool Get(T &out) const {
		const T *value = As<T>();
		if (value == nullptr) {
			return false;
		}
		out = *value;
		return true;
	}

	// Returns false without writing on an invalid ref or type mismatch.
	template <typename T> bool Set(const T &value) const {
		T *target = As<T>();
		if (target == nullptr) {
			return false;
		}
		*target = value;
		return true;
	}

  private:
	void *_data = nullptr;
	const FieldInfo *_info = nullptr;
};

} // namespace sowa
