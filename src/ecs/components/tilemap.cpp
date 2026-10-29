#include "ecs/components/tilemap.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <set>

#include "core/serialize/archive.hpp"

namespace sowa {
namespace {
bool positiveSize(Vector2 size) {
	return std::isfinite(size.x) && std::isfinite(size.y) && size.x > 0 && size.y > 0;
}

bool validRegion(Rect region) {
	return std::isfinite(region.x) && std::isfinite(region.y) && std::isfinite(region.w) &&
		   std::isfinite(region.h) && region.x >= 0 && region.y >= 0 && region.w >= 0 &&
		   region.h >= 0 && ((region.w == 0) == (region.h == 0));
}

void serializeCells(Archive &archive, const char *field, std::map<TileCell, i32> &cells) {
	struct Record {
		i32 x = 0, y = 0, tile = 0;
	};

	std::vector<Record> records;
	if (archive.IsSaving()) {
		for (const auto &[cell, tile] : cells) {
			records.push_back({cell.x, cell.y, tile});
		}
	}
	if (archive.List(field, records,
			[](Archive &entry, Record &record) {
				entry.Field("x", record.x);
				entry.Field("y", record.y);
				entry.Field("tile", record.tile);
			}) &&
		archive.IsLoading()) {
		cells.clear();
		for (const auto &record : records) {
			cells[{record.x, record.y}] = record.tile;
		}
	}
}

std::optional<TileCell> sheetGrid(Vector2 image, Vector2 tile, i32 margin, i32 spacing) {
	if (!positiveSize(image) || !positiveSize(tile) || margin < 0 || spacing < 0) {
		return {};
	}
	const double columns = std::max(0.0,
		std::floor((double(image.x) - 2.0 * margin + spacing) / (double(tile.x) + spacing)));
	const double rows = std::max(0.0,
		std::floor((double(image.y) - 2.0 * margin + spacing) / (double(tile.y) + spacing)));
	if (columns > 65536 || rows > 65536 || columns * rows > 65536) {
		return {};
	}
	return TileCell{static_cast<i32>(columns), static_cast<i32>(rows)};
}
} // namespace

const TileDefinition *Tilemap::FindTile(i32 id) const {
	const auto it = std::lower_bound(_tiles.begin(), _tiles.end(), id,
		[](const TileDefinition &tile, i32 value) { return tile.id < value; });
	return it != _tiles.end() && it->id == id ? &*it : nullptr;
}

TileLayer *Tilemap::Layer(i32 index) {
	return index >= 0 && static_cast<size_t>(index) < _layers.size() ? &_layers[index] : nullptr;
}

const TileLayer *Tilemap::Layer(i32 index) const {
	return index >= 0 && static_cast<size_t>(index) < _layers.size() ? &_layers[index] : nullptr;
}

i32 Tilemap::AddTexture(ResourceID resource, std::string name, Rect region) {
	if (!resource.IsValid() || _nextTileId == std::numeric_limits<i32>::max() ||
		!validRegion(region)) {
		return 0;
	}
	const i32 id = _nextTileId++;
	_tiles.push_back({id, std::move(name), resource, false, region});
	return id;
}

i32 Tilemap::AddScene(ResourceID resource, std::string name) {
	const i32 id = AddTexture(resource, std::move(name));
	if (id) {
		_tiles.back().scene = true;
	}
	return id;
}

std::vector<i32> Tilemap::AddSheet(ResourceID resource, Vector2 imageSize, Vector2 tileSize,
	i32 margin, i32 spacing) {
	std::vector<i32> ids;
	const auto grid = sheetGrid(imageSize, tileSize, margin, spacing);
	if (!resource.IsValid() || !grid || grid->x == 0 || grid->y == 0 || _nextSheetId == INT32_MAX ||
		grid->x * grid->y >= INT32_MAX - _nextTileId) {
		return ids;
	}
	const i32 sheet = _nextSheetId++;
	_sheets.push_back({sheet});
	UpdateSheet(sheet, resource, imageSize, tileSize, margin, spacing);
	for (i32 y = 0; y < grid->y; ++y) {
		for (i32 x = 0; x < grid->x; ++x) {
			ids.push_back(_sheets.back().tiles.at({x, y}));
		}
	}
	return ids;
}

const TileSheet *Tilemap::FindSheet(i32 id) const {
	const auto found = std::find_if(_sheets.begin(), _sheets.end(),
		[id](const TileSheet &sheet) { return sheet.id == id; });
	return found == _sheets.end() ? nullptr : &*found;
}

const TileSheet *Tilemap::SheetForTile(i32 id) const {
	if (id <= 0) {
		return nullptr;
	}
	for (const auto &sheet : _sheets) {
		for (const auto &[cell, tile] : sheet.tiles) {
			if (tile == id) {
				return &sheet;
			}
		}
	}
	return nullptr;
}

bool Tilemap::UpdateSheet(i32 id, ResourceID resource, Vector2 imageSize, Vector2 tileSize,
	i32 margin, i32 spacing) {
	auto *sheet = const_cast<TileSheet *>(FindSheet(id));
	const auto grid = sheetGrid(imageSize, tileSize, margin, spacing);
	if (!sheet || !resource.IsValid() || !grid) {
		return false;
	}
	i32 missing = 0;
	for (i32 y = 0; y < grid->y; ++y) {
		for (i32 x = 0; x < grid->x; ++x) {
			missing += !sheet->tiles.contains({x, y});
		}
	}
	if (missing >= INT32_MAX - _nextTileId) {
		return false;
	}
	sheet->resource = resource;
	sheet->imageSize = imageSize;
	sheet->tileSize = tileSize;
	sheet->margin = margin;
	sheet->spacing = spacing;
	for (i32 y = 0; y < grid->y; ++y) {
		for (i32 x = 0; x < grid->x; ++x) {
			if (!sheet->tiles.contains({x, y})) {
				sheet->tiles[{x, y}] =
					AddTexture(resource, std::to_string(x) + "," + std::to_string(y));
			}
		}
	}
	for (const auto &[cell, tileId] : sheet->tiles) {
		if (auto *tile = const_cast<TileDefinition *>(FindTile(tileId))) {
			tile->resource = resource;
			tile->available = cell.x < grid->x && cell.y < grid->y;
			tile->region = Rect(margin + cell.x * (double(tileSize.x) + spacing),
				margin + cell.y * (double(tileSize.y) + spacing), tileSize.x, tileSize.y);
		}
	}
	return true;
}

bool Tilemap::RefreshSheet(i32 id, Vector2 imageSize) {
	const auto *sheet = FindSheet(id);
	if (!sheet || sheet->imageSize == imageSize) {
		return false;
	}
	return UpdateSheet(id, sheet->resource, imageSize, sheet->tileSize, sheet->margin,
		sheet->spacing);
}

void Tilemap::detachTile(i32 id) {
	for (auto &sheet : _sheets) {
		for (auto &[cell, tile] : sheet.tiles) {
			if (tile == id) {
				tile = 0;
			}
		}
	}
}

bool Tilemap::SetTileSource(i32 id, ResourceID resource, bool scene, Rect region) {
	auto *tile = const_cast<TileDefinition *>(FindTile(id));
	if (!tile || !resource.IsValid() || !validRegion(region)) {
		return false;
	}
	detachTile(id);
	tile->resource = resource;
	tile->scene = scene;
	tile->available = true;
	tile->region = scene ? Rect{} : region;
	return true;
}

bool Tilemap::SetTileName(i32 id, std::string name) {
	auto *tile = const_cast<TileDefinition *>(FindTile(id));
	if (!tile || tile->name == name) {
		return false;
	}
	tile->name = std::move(name);
	return true;
}

bool Tilemap::RemoveTile(i32 id) {
	if (!FindTile(id)) {
		return false;
	}
	detachTile(id);
	std::erase_if(_tiles, [id](const auto &tile) { return tile.id == id; });
	for (auto &layer : _layers) {
		std::erase_if(layer.cells, [id](const auto &cell) { return cell.second == id; });
	}
	return true;
}

i32 Tilemap::AddLayer(std::string name) {
	if (_nextLayerId == std::numeric_limits<i32>::max()) {
		return -1;
	}
	_layers.push_back({_nextLayerId++, std::move(name), true, {}});
	return static_cast<i32>(_layers.size()) - 1;
}

bool Tilemap::RemoveLayer(i32 index) {
	if (!Layer(index)) {
		return false;
	}
	_layers.erase(_layers.begin() + index);
	return true;
}

bool Tilemap::MoveLayer(i32 from, i32 to) {
	if (!Layer(from) || !Layer(to) || from == to) {
		return false;
	}
	auto layer = std::move(_layers[from]);
	_layers.erase(_layers.begin() + from);
	_layers.insert(_layers.begin() + to, std::move(layer));
	return true;
}

i32 Tilemap::GetTile(i32 layer, i32 x, i32 y) const {
	const auto *data = Layer(layer);
	if (!data) {
		return 0;
	}
	const auto it = data->cells.find({x, y});
	return it == data->cells.end() ? 0 : it->second;
}

bool Tilemap::SetTile(i32 layer, i32 x, i32 y, i32 tile) {
	auto *data = Layer(layer);
	if (!data || (tile != 0 && !FindTile(tile)) || GetTile(layer, x, y) == tile) {
		return false;
	}
	if (tile == 0) {
		data->cells.erase({x, y});
	} else {
		data->cells[{x, y}] = tile;
	}
	return true;
}

bool Tilemap::ClearLayer(i32 layer) {
	auto *data = Layer(layer);
	if (!data || data->cells.empty()) {
		return false;
	}
	data->cells.clear();
	return true;
}

void Tilemap::Clear() {
	for (auto &layer : _layers) {
		layer.cells.clear();
	}
}

bool Tilemap::ValidCellSize() const {
	return positiveSize(cellSize);
}

std::optional<TileCell> Tilemap::LocalToCell(Vector2 point) const {
	if (!ValidCellSize()) {
		return {};
	}
	const double x = std::floor(double(point.x) / cellSize.x),
				 y = std::floor(double(point.y) / cellSize.y);
	if (!std::isfinite(x) || !std::isfinite(y) || x < INT32_MIN || x > INT32_MAX || y < INT32_MIN ||
		y > INT32_MAX) {
		return {};
	}
	return TileCell{static_cast<i32>(x), static_cast<i32>(y)};
}

Vector2 Tilemap::CellToLocal(i32 x, i32 y) const {
	return {(float(x) + .5f) * cellSize.x, (float(y) + .5f) * cellSize.y};
}

void Tilemap::Serialize(Archive &archive) {
	archive.Field("cellSize", cellSize);
	archive.Field("nextTileId", _nextTileId);
	archive.Field("nextLayerId", _nextLayerId);
	archive.Field("nextSheetId", _nextSheetId);
	archive.List("sheets", _sheets, [](Archive &a, TileSheet &sheet) {
		a.Field("id", sheet.id);
		a.ResourceField("resource", sheet.resource.value);
		a.Field("tileSize", sheet.tileSize);
		a.Field("margin", sheet.margin);
		a.Field("spacing", sheet.spacing);
		serializeCells(a, "tiles", sheet.tiles);
		if (a.IsLoading()) {
			sheet.imageSize = {};
		}
	});
	archive.List("tiles", _tiles, [](Archive &a, TileDefinition &tile) {
		a.Field("id", tile.id);
		a.Field("name", tile.name);
		a.ResourceField("resource", tile.resource.value);
		a.Field("scene", tile.scene);
		a.Field("x", tile.region.x);
		a.Field("y", tile.region.y);
		a.Field("width", tile.region.w);
		a.Field("height", tile.region.h);
	});
	archive.List("layers", _layers, [](Archive &a, TileLayer &layer) {
		a.Field("id", layer.id);
		a.Field("name", layer.name);
		a.Field("visible", layer.visible);
		serializeCells(a, "cells", layer.cells);
		std::erase_if(layer.cells, [](const auto &cell) { return cell.second == 0; });
	});
	if (archive.IsLoading()) {
		if (!ValidCellSize()) {
			archive.AddError("cellSize", "must be finite and positive");
			cellSize = {16, 16};
		}
		std::set<i32> ids;
		std::erase_if(_tiles, [&](const auto &tile) {
			const bool bad = tile.id <= 0 || tile.id == INT32_MAX || !ids.insert(tile.id).second ||
							 !tile.resource.IsValid() || !std::isfinite(tile.region.x) ||
							 !std::isfinite(tile.region.y) || !std::isfinite(tile.region.w) ||
							 !std::isfinite(tile.region.h) || tile.region.x < 0 ||
							 tile.region.y < 0 || tile.region.w < 0 || tile.region.h < 0 ||
							 ((tile.region.w == 0) != (tile.region.h == 0));
			if (bad) {
				archive.AddError("tiles", "invalid or duplicate tile definition");
			}
			return bad;
		});
		std::sort(_tiles.begin(), _tiles.end(),
			[](const auto &a, const auto &b) { return a.id < b.id; });
		_nextTileId = std::max(1, _nextTileId);
		for (const auto &tile : _tiles) {
			_nextTileId = std::max(_nextTileId, tile.id + 1);
		}
		ids.clear();
		std::set<i32> sheetTiles;
		_nextSheetId = std::max(1, _nextSheetId);
		std::erase_if(_sheets, [&](auto &sheet) {
			if (sheet.id <= 0 || sheet.id == INT32_MAX || !ids.insert(sheet.id).second ||
				!sheet.resource.IsValid() || !positiveSize(sheet.tileSize) || sheet.margin < 0 ||
				sheet.spacing < 0) {
				archive.AddError("sheets", "invalid or duplicate sheet");
				return true;
			}
			_nextSheetId = std::max(_nextSheetId, sheet.id + 1);
			std::erase_if(sheet.tiles, [&](const auto &slot) {
				const auto &[cell, id] = slot;
				const auto *tile = FindTile(id);
				const bool bad =
					cell.x < 0 || cell.y < 0 || cell.x >= 65536 || cell.y >= 65536 ||
					(id != 0 && (!tile || tile->scene || !sheetTiles.insert(id).second));
				if (bad) {
					archive.AddError("sheets", "invalid sheet tile");
				}
				return bad;
			});
			return false;
		});
		ids.clear();
		_nextLayerId = std::max(1, _nextLayerId);
		std::erase_if(_layers, [&](const auto &layer) {
			const bool bad = layer.id < 0 || layer.id == INT32_MAX || !ids.insert(layer.id).second;
			if (bad) {
				archive.AddError("layers", "invalid or duplicate layer id");
			}
			return bad;
		});
		for (auto &layer : _layers) {
			_nextLayerId = std::max(_nextLayerId, layer.id + 1);
			std::erase_if(layer.cells, [&](const auto &cell) {
				if (FindTile(cell.second)) {
					return false;
				}
				archive.AddError("cells", "unknown tile id");
				return true;
			});
		}
	}
}

Rect TileUV(const TileDefinition &tile, Vector2 size, bool flipped) {
	if (!tile.available || !positiveSize(size)) {
		return {};
	}
	const Rect r =
		tile.region.w == 0 && tile.region.h == 0 ? Rect(0, 0, size.x, size.y) : tile.region;
	if (r.x < 0 || r.y < 0 || r.Right() > size.x || r.Top() > size.y) {
		return {};
	}
	return flipped ? Rect(r.x / size.x, 1.f - r.Top() / size.y, r.w / size.x, r.h / size.y)
				   : Rect(r.x / size.x, r.Top() / size.y, r.w / size.x, -r.h / size.y);
}
} // namespace sowa
