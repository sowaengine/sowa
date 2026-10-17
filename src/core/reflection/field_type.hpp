#pragma once

#include <cstddef>
#include <initializer_list>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include "core/reflection/enum_constant.hpp" // IWYU pragma: export
#include "core/types.hpp"
#include "math/color.hpp"
#include "math/vector2.hpp"
#include "math/vector3.hpp"

namespace sowa {

// Storage types supported by reflection. Native and runtime registration must use
// the same layout for each type: String is std::string and EntityRef is sowa::Entity.
// ResolveFieldTypeId in field_registry.hpp maps these layouts to flecs types.
enum class FieldType {
	Bool,
	I32,
	U32,
	F32,
	F64,
	String,
	EntityRef,
	Vector2,
	Vector3,
	Color,
	// i32 storage with names and constants supplied by FieldInfo or EnumTraits.
	Enum,
	// ResourceID storage. The ID identifies a resource; registry Get<T>() checks its kind.
	// FieldTypeOf<ResourceID> lives in resource/resource_id.hpp to preserve layering.
	ResourceRef,
};

// Serialize persists a field; Expose makes it available to scripts and the inspector.
// None keeps storage internal. Native members omitted from reflection are also internal.
enum class FieldFlags : u32 {
	None = 0,
	Serialize = 1 << 0,
	Expose = 1 << 1,
	Default = Serialize | Expose,
};

constexpr FieldFlags operator|(FieldFlags a, FieldFlags b) {
	return static_cast<FieldFlags>(static_cast<u32>(a) | static_cast<u32>(b));
}

constexpr FieldFlags operator&(FieldFlags a, FieldFlags b) {
	return static_cast<FieldFlags>(static_cast<u32>(a) & static_cast<u32>(b));
}

constexpr bool HasFlag(FieldFlags value, FieldFlags flag) {
	return (static_cast<u32>(value) & static_cast<u32>(flag)) == static_cast<u32>(flag);
}

// A field as *described* at registration time. Offsets aren't part of this:
// they belong to the resolved layout and are read back into FieldInfo below.
struct FieldDesc {
	std::string name;
	FieldType type;
	FieldFlags flags = FieldFlags::Default;

	// FieldType::Enum only, ignored otherwise: which enum this field is, and
	// its named values. A data-described component supplies these inline; the
	// native path fills them in from EnumTraits<T>.
	std::string enumName;
	std::vector<EnumConstant> enumConstants;
	// Empty accepts any resource type. Used by resource pickers.
	std::string resourceType;
};

// Registered field description with its resolved byte offset, used by serialization,
// script bindings, and the inspector.
struct FieldInfo {
	std::string name;
	FieldType type = FieldType::F32;
	FieldFlags flags = FieldFlags::Default;
	size_t offset = 0;

	// FieldType::Enum only; empty otherwise.
	std::string enumName;
	std::vector<EnumConstant> enumConstants;
	// Empty accepts any resource type. Used by resource pickers.
	std::string resourceType;

	bool IsSerialized() const {
		return HasFlag(flags, FieldFlags::Serialize);
	}

	bool IsExposed() const {
		return HasFlag(flags, FieldFlags::Expose);
	}

	// Name of the constant equal to value, or nullptr if no constant has it.
	// An enum field can legitimately hold an unnamed value (a cast from an
	// arbitrary int), so callers have to handle the miss.
	const std::string *FindEnumName(i32 value) const {
		for (const EnumConstant &constant : enumConstants) {
			if (constant.value == value) {
				return &constant.name;
			}
		}
		return nullptr;
	}

	// Value of the constant called name. Returns false, leaving out
	// untouched, if there's no such constant - which is what deserializing a
	// scene saved against an older version of the enum looks like.
	bool FindEnumValue(std::string_view name, i32 &out) const {
		for (const EnumConstant &constant : enumConstants) {
			if (constant.name == name) {
				out = constant.value;
				return true;
			}
		}
		return false;
	}
};

// Maps supported C++ member types to reflected storage types. Unsupported types fail at compile
// time.
template <typename T> struct FieldTypeOf {
	static constexpr bool Valid = false;
	static constexpr FieldType Value = FieldType::F32;
};

#define SOWA_FIELD_TYPE_OF(CppType, FieldTypeValue)                                                \
	template <> struct FieldTypeOf<CppType> {                                                      \
		static constexpr bool Valid = true;                                                        \
		static constexpr FieldType Value = FieldTypeValue;                                         \
	}

SOWA_FIELD_TYPE_OF(bool, FieldType::Bool);
SOWA_FIELD_TYPE_OF(i32, FieldType::I32);
SOWA_FIELD_TYPE_OF(u32, FieldType::U32);
SOWA_FIELD_TYPE_OF(f32, FieldType::F32);
SOWA_FIELD_TYPE_OF(f64, FieldType::F64);
SOWA_FIELD_TYPE_OF(std::string, FieldType::String);
SOWA_FIELD_TYPE_OF(Vector2, FieldType::Vector2);
SOWA_FIELD_TYPE_OF(Vector3, FieldType::Vector3);
SOWA_FIELD_TYPE_OF(Color, FieldType::Color);

// FieldTypeOf<Entity> stays in ecs/entity.hpp - it's the one field type that
// genuinely belongs to the ECS, and putting it here would make every reflected
// type drag in flecs.

// Byte offset derived from a pointer-to-member. Shared by component and resource reflection.
template <typename T, typename M> size_t FieldOffsetOf(M T::*member) {
	alignas(T) static unsigned char storage[sizeof(T)];
	const T *object = reinterpret_cast<const T *>(storage);
	return static_cast<size_t>(
		reinterpret_cast<const unsigned char *>(&(object->*member)) - storage);
}

// EnumTraits supplies named constants for i32 enum storage. Unreflected enum fields fail at
// compile time.
template <typename T> struct EnumTraits {
	static constexpr bool Valid = false;
};

// A name/value pair as written in a SOWA_REFLECT_ENUM declaration. Takes the
// enumerator itself rather than an int, so a typo names a different constant
// instead of silently compiling to a wrong number.
template <typename T> struct EnumEntry {
	const char *name;
	T value;
};

template <typename T>
std::vector<EnumConstant> MakeEnumConstants(std::initializer_list<EnumEntry<T>> entries) {
	std::vector<EnumConstant> constants;
	constants.reserve(entries.size());
	for (const EnumEntry<T> &entry : entries) {
		constants.push_back(EnumConstant{entry.name, static_cast<i32>(entry.value)});
	}
	return constants;
}

// Declare SOWA_REFLECT_ENUM at namespace sowa scope. Reflected enums must have i32-sized
// storage.
#define SOWA_REFLECT_ENUM(EnumT, ...)                                                              \
	template <> struct FieldTypeOf<EnumT> {                                                        \
		static constexpr bool Valid = true;                                                        \
		static constexpr FieldType Value = FieldType::Enum;                                        \
	};                                                                                             \
	template <> struct EnumTraits<EnumT> {                                                         \
		static_assert(std::is_enum_v<EnumT>, #EnumT " is not an enum type");                       \
		static_assert(sizeof(EnumT) == sizeof(i32),                                                \
			#EnumT " must have a 32-bit underlying type to be "                                    \
				   "reflected (see FieldType::Enum)");                                             \
		static constexpr bool Valid = true;                                                        \
		static const char *Name() {                                                                \
			return #EnumT;                                                                         \
		}                                                                                          \
		static std::vector<EnumConstant> Constants() {                                             \
			return MakeEnumConstants<EnumT>({__VA_ARGS__});                                        \
		}                                                                                          \
	}

} // namespace sowa
