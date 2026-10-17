#pragma once

// Build/version info embedded at build time.
// Implementation is generated from build_info.cpp.in - see cmake/GenerateBuildInfo.cmake.
namespace sowa::BuildInfo {

// Engine version (CMake project version).
const char *Version();

// Abbreviated commit hash of the git HEAD this was built from, or "unknown"
// if it wasn't built from a git checkout.
const char *GitHash();

// Git branch this was built from, or "unknown" if it wasn't built from a git checkout.
const char *GitBranch();

// True if the working tree had uncommitted changes when this was built.
bool IsDirty();

} // namespace sowa::BuildInfo
