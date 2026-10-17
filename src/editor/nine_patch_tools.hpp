#pragma once

#include "ecs/components/nine_patch_rect.hpp"
#include <string_view>

namespace sowa::editor {

// Corner coordinates are 0/1 in local space (Y up).
NinePatchRect ResizeNinePatchRect(const NinePatchRect &before, Vector2 delta, Vector2 corner);

// Converts a top-left texture point to one padding value without crossing its opposite guide.
f32 NinePatchPaddingAt(std::string_view field, Vector2 point, Vector2 textureSize, f32 opposite);

} // namespace sowa::editor
