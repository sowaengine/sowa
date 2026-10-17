#pragma once

#include "ecs/components/tilemap.hpp"

namespace sowa {
class ResourceRegistry;
}

namespace sowa::editor {
// Returns the clicked tile ID, if any.
std::optional<i32> DrawTilemapPalette(const Tilemap &map, ResourceRegistry &resources,
	i32 selected);
} // namespace sowa::editor
