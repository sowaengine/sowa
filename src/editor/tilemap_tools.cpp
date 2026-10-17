#include "editor/tilemap_tools.hpp"
#include "core/filesystem/file_system.hpp"
#include "editor/filesystem_browser.hpp"
#include "resource/image_texture.hpp"
#include "resource/resource_registry.hpp"
#include "scene/packed_scene.hpp"
#include <cmath>
#include <cstdlib>

namespace sowa::editor {
std::string TileAssetPath(ResourceRegistry &resources, ResourceID id) {
	if (auto image = resources.Get<ImageTexture>(id)) {
		return image->path;
	}
	if (auto scene = resources.Get<PackedScene>(id)) {
		return scene->path;
	}
	return {};
}

TileAsset LoadTileAsset(ResourceRegistry &resources, FileSystem &fs, std::string_view input,
	bool scene, std::string_view currentScene) {
	auto canonical = [&](std::string_view value) {
		const auto result = fs.Resolve(value);
		if (!result) {
			return std::string{};
		}
		const auto relative = std::filesystem::path(result.value.path).lexically_normal();
		if (relative.is_absolute() || relative.empty() || *relative.begin() == "..") {
			return std::string{};
		}
		return result.value.scheme + "://" + relative.generic_string();
	};
	const std::string path = canonical(input);
	if (!path.starts_with("res://") ||
		KindFromName(path, false) != (scene ? AssetKind::Scene : AssetKind::Image)) {
		return {{}, false, "Choose a project image or scene file."};
	}
	if (scene && path == canonical(currentScene)) {
		return {{}, false, "A scene cannot use itself as a tile."};
	}
	TileAsset result;
	resources.Each([&](ResourceID id, Resource &, std::string_view) {
		if (canonical(TileAssetPath(resources, id)) == path) {
			result.id = id;
		}
	});
	if (!result.id.IsValid()) {
		if (scene) {
			if (auto resource = resources.Create<PackedScene>()) {
				resource->path = path;
				result.id = resource->Id();
			}
		} else {
			if (auto resource = resources.Create<ImageTexture>()) {
				resource->path = path;
				resource->filter = TextureFilter::Nearest;
				result.id = resource->Id();
			}
		}
		result.created = result.id.IsValid();
	}
	const auto image = resources.Get<ImageTexture>(result.id);
	const auto packed = resources.Get<PackedScene>(result.id);
	if (result.created || (image && image->Width() == 0) || (packed && !packed->IsLoaded())) {
		resources.Reload(result.id);
	}
	if ((scene && packed && packed->IsLoaded()) ||
		(!scene && image && image->Width() > 0 && image->Height() > 0)) {
		return result;
	}
	if (result.created) {
		resources.Remove(result.id);
	}
	return {{}, false, "Could not load the source file."};
}

std::optional<TileCell> TilemapViewportCell(const Tilemap &map, const glm::mat4 &world,
	const glm::mat4 &vp, const Rect &image, Vector2 mouse) {
	if (image.w <= 0 || image.h <= 0) {
		return {};
	}
	const auto matrix = vp * world;
	const float determinant = glm::determinant(matrix);
	if (!std::isfinite(determinant) || determinant == 0.f) {
		return {};
	}
	const auto local = glm::inverse(matrix) * glm::vec4(2.f * (mouse.x - image.x) / image.w - 1.f,
												  1.f - 2.f * (mouse.y - image.y) / image.h, 0, 1);
	if (!std::isfinite(local.w) || local.w == 0) {
		return {};
	}
	return map.LocalToCell({local.x / local.w, local.y / local.w});
}

bool PaintTileLine(Tilemap &map, i32 layer, TileCell from, TileCell to, i32 tile) {
	int64_t x = from.x, y = from.y;
	const int64_t dx = std::abs(int64_t(to.x) - x), dy = -std::abs(int64_t(to.y) - y);
	// A discontinuity from a camera jump must not allocate billions of cells.
	if (dx > 65536 || -dy > 65536) {
		return map.SetTile(layer, to.x, to.y, tile);
	}
	const int64_t sx = x < to.x ? 1 : -1, sy = y < to.y ? 1 : -1;
	int64_t error = dx + dy;
	bool changed = false;
	for (;;) {
		changed = map.SetTile(layer, static_cast<i32>(x), static_cast<i32>(y), tile) || changed;
		if (x == to.x && y == to.y) {
			return changed;
		}
		const int64_t e = 2 * error;
		if (e >= dy) {
			error += dy;
			x += sx;
		}
		if (e <= dx) {
			error += dx;
			y += sy;
		}
	}
}
} // namespace sowa::editor
