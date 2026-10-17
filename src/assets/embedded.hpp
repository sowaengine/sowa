#pragma once

#include <span>

namespace sowa::assets {

std::span<const unsigned char> DefaultFont();
std::span<const unsigned char> DefaultFontLicense();
std::span<const unsigned char> SpriteShader();
std::span<const unsigned char> FullscreenShader();
std::span<const unsigned char> EditorIcons();
std::span<const unsigned char> EditorIconsLicense();

} // namespace sowa::assets
