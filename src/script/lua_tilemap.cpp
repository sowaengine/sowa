#include "script/lua_tilemap.hpp"
#include "ecs/components/tilemap.hpp"
#include "scene/tilemap_scenes.hpp"

namespace sowa {
namespace {
Tilemap *live(ComponentView self) {
	return self.entity.IsAlive() && self.type.IsValid() && self.type.Name() == "Tilemap"
			   ? self.entity.Get<Tilemap>()
			   : nullptr;
}
} // namespace

sol::object TilemapMethod(ComponentView &view, const std::string &name, sol::this_state s) {
	if (!live(view)) {
		return sol::make_object(s, sol::lua_nil);
	}
	if (name == "get_tile") {
		return sol::make_object(s, [](ComponentView self, i32 layer, i32 x, i32 y) {
			auto *map = live(self);
			return map ? map->GetTile(layer, x, y) : 0;
		});
	}
	if (name == "set_tile") {
		return sol::make_object(s, [](ComponentView self, i32 layer, i32 x, i32 y, i32 tile) {
			auto *map = live(self);
			return map && map->SetTile(layer, x, y, tile);
		});
	}
	if (name == "erase_tile") {
		return sol::make_object(s, [](ComponentView self, i32 layer, i32 x, i32 y) {
			auto *map = live(self);
			return map && map->EraseTile(layer, x, y);
		});
	}
	if (name == "clear_layer") {
		return sol::make_object(s, [](ComponentView self, i32 layer) {
			auto *map = live(self);
			return map && map->ClearLayer(layer);
		});
	}
	if (name == "clear") {
		return sol::make_object(s, [](ComponentView self) {
			if (auto *map = live(self)) {
				map->Clear();
			}
		});
	}
	if (name == "add_layer") {
		return sol::make_object(s, [](ComponentView self, std::string name) {
			auto *map = live(self);
			return map ? map->AddLayer(std::move(name)) : -1;
		});
	}
	if (name == "remove_layer") {
		return sol::make_object(s, [](ComponentView self, i32 layer) {
			auto *map = live(self);
			return map && map->RemoveLayer(layer);
		});
	}
	if (name == "move_layer") {
		return sol::make_object(s, [](ComponentView self, i32 from, i32 to) {
			auto *map = live(self);
			return map && map->MoveLayer(from, to);
		});
	}
	if (name == "layer_count") {
		return sol::make_object(s, [](ComponentView self) {
			auto *map = live(self);
			return map ? map->Layers().size() : size_t(0);
		});
	}
	if (name == "set_layer_visible") {
		return sol::make_object(s, [](ComponentView self, i32 layer, bool visible) {
			auto *map = live(self);
			auto *data = map ? map->Layer(layer) : nullptr;
			if (!data) {
				return false;
			}
			data->visible = visible;
			return true;
		});
	}
	if (name == "set_layer_name") {
		return sol::make_object(s, [](ComponentView self, i32 layer, std::string name) {
			auto *map = live(self);
			auto *data = map ? map->Layer(layer) : nullptr;
			if (!data) {
				return false;
			}
			data->name = std::move(name);
			return true;
		});
	}
	if (name == "get_layer") {
		return sol::make_object(s,
			[](ComponentView self, i32 layer, sol::this_state state) -> sol::object {
				auto *map = live(self);
				auto *data = map ? map->Layer(layer) : nullptr;
				if (!data) {
					return sol::make_object(state, sol::lua_nil);
				}
				sol::state_view lua(state);
				auto out = lua.create_table();
				out["name"] = data->name;
				out["visible"] = data->visible;
				out["id"] = data->id;
				return out;
			});
	}
	if (name == "add_texture") {
		return sol::make_object(s, [](ComponentView self, u64 id, sol::optional<std::string> name) {
			auto *map = live(self);
			return map ? map->AddTexture(ResourceID{id}, name.value_or("")) : 0;
		});
	}
	if (name == "add_atlas_tile") {
		return sol::make_object(s, [](ComponentView self, u64 id, f32 x, f32 y, f32 w, f32 h,
									   sol::optional<std::string> name) {
			auto *map = live(self);
			return map ? map->AddTexture(ResourceID{id}, name.value_or(""), Rect(x, y, w, h)) : 0;
		});
	}
	if (name == "add_scene") {
		return sol::make_object(s, [](ComponentView self, u64 id, sol::optional<std::string> name) {
			auto *map = live(self);
			return map ? map->AddScene(ResourceID{id}, name.value_or("")) : 0;
		});
	}
	if (name == "add_sheet") {
		return sol::make_object(s,
			[](ComponentView self, u64 id, Vector2 imageSize, Vector2 tileSize,
				sol::optional<i32> margin, sol::optional<i32> spacing) {
				auto *map = live(self);
				return sol::as_table(map ? map->AddSheet(ResourceID{id}, imageSize, tileSize,
											   margin.value_or(0), spacing.value_or(0))
										 : std::vector<i32>{});
			});
	}
	if (name == "remove_tile") {
		return sol::make_object(s, [](ComponentView self, i32 id) {
			auto *map = live(self);
			return map && map->RemoveTile(id);
		});
	}
	if (name == "set_tile_source") {
		return sol::make_object(s,
			[](ComponentView self, i32 tile, u64 resource, sol::optional<bool> scene) {
				auto *map = live(self);
				return map && map->SetTileSource(tile, ResourceID{resource}, scene.value_or(false));
			});
	}
	if (name == "set_tile_name") {
		return sol::make_object(s, [](ComponentView self, i32 tile, std::string name) {
			auto *map = live(self);
			return map && map->SetTileName(tile, std::move(name));
		});
	}
	if (name == "get_tile_definition") {
		return sol::make_object(s,
			[](ComponentView self, i32 id, sol::this_state state) -> sol::object {
				auto *map = live(self);
				const auto *tile = map ? map->FindTile(id) : nullptr;
				if (!tile) {
					return sol::make_object(state, sol::lua_nil);
				}
				sol::state_view lua(state);
				auto out = lua.create_table();
				out["id"] = tile->id;
				out["name"] = tile->name;
				out["resource"] = tile->resource.value;
				out["scene"] = tile->scene;
				const auto *sheet = map->SheetForTile(id);
				out["sheet"] = sheet ? sheet->id : 0;
				return out;
			});
	}
	if (name == "update_sheet") {
		return sol::make_object(s,
			[](ComponentView self, i32 sheet, u64 resource, Vector2 imageSize, Vector2 tileSize,
				sol::optional<i32> margin, sol::optional<i32> spacing) {
				auto *map = live(self);
				return map && map->UpdateSheet(sheet, ResourceID{resource}, imageSize, tileSize,
								  margin.value_or(0), spacing.value_or(0));
			});
	}
	if (name == "cell_to_local") {
		return sol::make_object(s, [](ComponentView self, i32 x, i32 y) {
			auto *map = live(self);
			return map ? map->CellToLocal(x, y) : Vector2{};
		});
	}
	if (name == "local_to_cell") {
		return sol::make_object(s,
			[](ComponentView self, Vector2 point, sol::this_state state) -> sol::object {
				auto *map = live(self);
				const auto cell = map ? map->LocalToCell(point) : std::nullopt;
				if (!cell) {
					return sol::make_object(state, sol::lua_nil);
				}
				sol::state_view lua(state);
				auto out = lua.create_table();
				out["x"] = cell->x;
				out["y"] = cell->y;
				return out;
			});
	}
	if (name == "get_used_cells") {
		return sol::make_object(s, [](ComponentView self, i32 layer, sol::this_state state) {
			sol::state_view lua(state);
			auto out = lua.create_table();
			auto *map = live(self);
			auto *data = map ? map->Layer(layer) : nullptr;
			if (data) {
				for (const auto &[cell, tile] : data->cells) {
					auto item = lua.create_table();
					item["x"] = cell.x;
					item["y"] = cell.y;
					item["tile"] = tile;
					out.add(item);
				}
			}
			return out;
		});
	}
	if (name == "get_tile_entity") {
		return sol::make_object(s,
			[](ComponentView self, i32 layer, i32 x, i32 y, sol::this_state state) -> sol::object {
				const Entity entity =
					live(self) ? GetTileSceneEntity(self.entity, layer, x, y) : Entity{};
				return entity.IsAlive() ? sol::make_object(state, entity)
										: sol::make_object(state, sol::lua_nil);
			});
	}
	return sol::make_object(s, sol::lua_nil);
}
} // namespace sowa
