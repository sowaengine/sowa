#pragma once

#include "core/types.hpp"

namespace sowa {

// Editor command identity preserved across snapshot clones. Unregistered so it never
// appears in scene files.
struct EditorEntityId {
	u32 high = 0;
	u32 low = 0;

	u64 Value() const {
		return (static_cast<u64>(high) << 32u) | static_cast<u64>(low);
	}

	void Set(u64 value) {
		high = static_cast<u32>(value >> 32u);
		low = static_cast<u32>(value);
	}
};

} // namespace sowa
