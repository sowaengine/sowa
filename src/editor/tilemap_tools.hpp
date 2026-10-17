#pragma once
#include "ecs/components/tilemap.hpp"
#include <glm/glm.hpp>

namespace sowa {
class FileSystem;
class ResourceRegistry;
} // namespace sowa

namespace sowa::editor {
struct TileAsset {
	ResourceID id;
	bool created = false;
	std::string error;
};

TileAsset LoadTileAsset(ResourceRegistry &resources, FileSystem &fs, std::string_view path,
	bool scene, std::string_view currentScene);
std::string TileAssetPath(ResourceRegistry &resources, ResourceID id);
std::optional<TileCell> TilemapViewportCell(const Tilemap &map, const glm::mat4 &world,
	const glm::mat4 &viewProjection, const Rect &image, Vector2 mouse);
// Bresenham interpolation keeps a fast drag from leaving holes.
bool PaintTileLine(Tilemap &map, i32 layer, TileCell from, TileCell to, i32 tile);
} // namespace sowa::editor
