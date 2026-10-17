#pragma once

#include <string>

#include "core/types.hpp"

namespace sowa {

// Named enum value shared by component reflection, resource reflection, and serialization.
// The archive writes names so files remain valid when enum constants are renumbered.
struct EnumConstant {
	std::string name;
	i32 value = 0;
};

} // namespace sowa
