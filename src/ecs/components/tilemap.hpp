#pragma once

#include <map>
#include <optional>
#include <string>
#include <vector>

#include "core/types.hpp"
#include "math/rect.hpp"
#include "math/vector2.hpp"
#include "resource/resource_id.hpp"

namespace sowa {
class Archive;

struct TileCell {
	i32 x = 0, y = 0;
	auto operator<=>(const TileCell &) const = default;
};

struct TileDefinition {
	i32 id = 0; // zero is the empty cell; IDs are never reused
	std::string name;
	ResourceID resource; // ImageTexture or PackedScene, according to scene
	bool scene = false;
	// Pixel rectangle measured from the top-left of the source image.
	// A zero size samples the entire image.
	Rect region;
	bool available = true; // derived: the slice fits the current sheet grid
};

struct TileLayer {
	i32 id = 0; // stable across reordering
	std::string name = "Ground";
	bool visible = true;
	std::map<TileCell, i32> cells;
};

struct TileSheet {
	i32 id = 0;
	ResourceID resource;
	Vector2 tileSize{16, 16};
	i32 margin = 0;
	i32 spacing = 0;
	// Preserve coordinate IDs during resize. Zero marks a removed tile and must stay removed.
	std::map<TileCell, i32> tiles;
	Vector2 imageSize; // last synchronized dimensions
};

// Sparse, orthogonal, Y-up grid. Coordinates and layer indices are zero-based.
// Cell (0,0) occupies [0,cellSize.x) x [0,cellSize.y) in local space.
// Resources follow the scene's usual Own/Disown lifetime contract.
class Tilemap {
  public:
	Vector2 cellSize{16.f, 16.f};

	const std::vector<TileDefinition> &Tiles() const {
		return _tiles;
	}

	const std::vector<TileLayer> &Layers() const {
		return _layers;
	}

	const TileDefinition *FindTile(i32 id) const;

	const std::vector<TileSheet> &Sheets() const {
		return _sheets;
	}

	const TileSheet *FindSheet(i32 id) const;
	const TileSheet *SheetForTile(i32 tile) const;
	TileLayer *Layer(i32 index);
	const TileLayer *Layer(i32 index) const;
	i32 AddTexture(ResourceID texture, std::string name = {}, Rect region = {});
	i32 AddScene(ResourceID scene, std::string name = {});
	// Returns IDs in row-major order, starting at the sheet's top-left.
	std::vector<i32> AddSheet(ResourceID texture, Vector2 imageSize, Vector2 tileSize,
		i32 margin = 0, i32 spacing = 0);
	bool UpdateSheet(i32 id, ResourceID texture, Vector2 imageSize, Vector2 tileSize,
		i32 margin = 0, i32 spacing = 0);
	bool RefreshSheet(i32 id, Vector2 imageSize);
	// Changes all painted uses without changing the tile ID. Editing a sheet
	// tile individually detaches it from subsequent sheet updates.
	bool SetTileSource(i32 id, ResourceID resource, bool scene = false, Rect region = {});
	bool SetTileName(i32 id, std::string name);
	bool RemoveTile(i32 id); // also erases all uses
	i32 AddLayer(std::string name);
	bool RemoveLayer(i32 index);
	bool MoveLayer(i32 from, i32 to);
	i32 GetTile(i32 layer, i32 x, i32 y) const;
	bool SetTile(i32 layer, i32 x, i32 y, i32 tile);

	bool EraseTile(i32 layer, i32 x, i32 y) {
		return SetTile(layer, x, y, 0);
	}

	bool ClearLayer(i32 layer);
	void Clear();
	bool ValidCellSize() const;
	std::optional<TileCell> LocalToCell(Vector2 point) const;
	Vector2 CellToLocal(i32 x, i32 y) const; // center of cell
	void Serialize(Archive &archive);

  private:
	i32 _nextTileId = 1;
	i32 _nextLayerId = 1;
	i32 _nextSheetId = 1;
	std::vector<TileDefinition> _tiles;
	std::vector<TileSheet> _sheets;
	std::vector<TileLayer> _layers{{}};
	void detachTile(i32 id);
};

// Normalized UVs, honoring the image loader's optional vertical flip.
Rect TileUV(const TileDefinition &tile, Vector2 imageSize, bool flipped = true);
} // namespace sowa
