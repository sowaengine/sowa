#pragma once

#include <string>
#include <vector>

#include "core/log.hpp"
#include "core/reflection/field_type.hpp"
#include "core/types.hpp"

namespace sowa {

// Describes fields through chained Field() calls after ResourceRegistry::RegisterType.
// Member pointers supply offsets and types. Updates take effect immediately; failed
// registration returns an inert builder. Undescribed members remain hidden from
// serialization, the inspector, and scripts.
template <typename T> class ResourceTypeBuilder {
  public:
	ResourceTypeBuilder(std::vector<FieldInfo> *fields, bool ok)
		: _fields(fields), _ok(ok && fields != nullptr) {
	}

	// Describes member as a reflected field named name. Chainable.
	template <typename M>
	ResourceTypeBuilder &Field(const std::string &name, M T::*member,
		FieldFlags flags = FieldFlags::Default) {
		static_assert(FieldTypeOf<M>::Valid,
			"member's type is not a supported field type (see FieldType)");
		// A resource outlives the scene whose entities it could name, and a
		// path saved into an asset file would dangle the moment that scene
		// changed. Components are where entity references belong.
		static_assert(FieldTypeOf<M>::Value != FieldType::EntityRef,
			"a resource can't hold an entity reference - resources outlive scenes");

		if (!_ok) {
			return *this;
		}

		if (name.empty()) {
			Log::Error("ResourceTypeBuilder: a field needs a name");
			return *this;
		}
		// Two fields under one name would collide on a single archive key, so
		// one would silently overwrite the other on save and both would read
		// the same value back.
		for (const FieldInfo &existing : *_fields) {
			if (existing.name == name) {
				Log::Error("ResourceTypeBuilder: field '{}' is already described", name);
				return *this;
			}
		}

		FieldInfo info;
		info.name = name;
		info.type = FieldTypeOf<M>::Value;
		info.flags = flags;
		info.offset = FieldOffsetOf(member);
		if constexpr (FieldTypeOf<M>::Value == FieldType::Enum) {
			static_assert(EnumTraits<M>::Valid, "enum member has no SOWA_REFLECT_ENUM declaration");
			info.enumName = EnumTraits<M>::Name();
			info.enumConstants = EnumTraits<M>::Constants();
		}
		_fields->push_back(std::move(info));
		return *this;
	}

	template <typename M>
	ResourceTypeBuilder &ResourceField(const std::string &name, M T::*member,
		const std::string &resourceType, FieldFlags flags = FieldFlags::Default) {
		static_assert(FieldTypeOf<M>::Value == FieldType::ResourceRef);
		const size_t previousCount = _fields ? _fields->size() : 0;
		Field(name, member, flags);
		if (_ok && _fields->size() > previousCount) {
			_fields->back().resourceType = resourceType;
		}
		return *this;
	}

	// False if RegisterType failed, in which case Field() does nothing.
	bool IsValid() const {
		return _ok;
	}

	explicit operator bool() const {
		return _ok;
	}

  private:
	std::vector<FieldInfo> *_fields = nullptr;
	bool _ok = false;
};

} // namespace sowa
