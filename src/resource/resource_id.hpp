#pragma once

#include <cstddef>
#include <functional>

#include "core/reflection/field_type.hpp"
#include "core/types.hpp"

namespace sowa {

// 64-bit resource identity used in files, ECS fields, and Lua. Resolve through the
// registry when accessing data; Ref<Resource> provides temporary C++ access.
// IDs do not retain resources, and stale IDs resolve to nothing. Random IDs reduce
// collisions between independently authored scene files.
struct ResourceID {
	// 0 is reserved for "no resource" and is never handed out.
	u64 value = 0;

	bool IsValid() const {
		return value != 0;
	}

	explicit operator bool() const {
		return IsValid();
	}

	bool operator==(const ResourceID &other) const {
		return value == other.value;
	}

	bool operator!=(const ResourceID &other) const {
		return value != other.value;
	}
};

// ResourceID reflection traits live beside the type to keep core independent of resources.
SOWA_FIELD_TYPE_OF(ResourceID, FieldType::ResourceRef);

} // namespace sowa

template <> struct std::hash<sowa::ResourceID> {
	size_t operator()(const sowa::ResourceID &id) const noexcept {
		return std::hash<sowa::u64>{}(id.value);
	}
};
