#pragma once

#include <filesystem>

namespace sowa {

// Absolute executable path from the OS, with symlinks resolved.
// Empty on failure or on Web.
std::filesystem::path ExecutablePath();

} // namespace sowa
