#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace sowa::build {

// Builds a macOS .icns icon from an image file (PNG, JPEG, ... anything
// stb_image decodes). The image is centered on a transparent square and
// scaled to every size Finder and the Dock use, each stored as PNG.
// Returns the .icns bytes, or nullopt with error set.
std::optional<std::string> MakeIcns(std::string_view image, std::string &error);

} // namespace sowa::build
