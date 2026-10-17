#include "scene/tilemap_scenes.hpp"

#include <map>
#include <tuple>
#include <vector>

#include "ecs/components/render_order.hpp"
#include "ecs/components/transform2d.hpp"
#include "ecs/world.hpp"
#include "resource/resource_registry.hpp"
#include "scene/packed_scene.hpp"
#include "scene/scene.hpp"

namespace sowa {
bool IsGeneratedTileEntity(Entity entity) {
	for (Entity current = entity; current.IsAlive(); current = current.GetParent()) {
		if (current.Has<TileSceneCell>()) {
			return true;
		}
	}
	return false;
}

Entity GetTileSceneEntity(Entity owner, i32 layer, i32 x, i32 y) {
	const auto *map = owner.Get<Tilemap>();
	const auto *data = map ? map->Layer(layer) : nullptr;
	const auto *tile = map ? map->FindTile(map->GetTile(layer, x, y)) : nullptr;
	if (!data || !tile || !tile->scene) {
		return {};
	}
	for (Entity child : owner.GetChildren()) {
		const auto *cell = child.Get<TileSceneCell>();
		if (cell && cell->layerId == data->id && cell->cell == TileCell{x, y} &&
			cell->tile == tile->id && cell->resource == tile->resource) {
			const auto children = child.GetChildren();
			return children.empty() ? Entity{} : children.front();
		}
	}
	return {};
}

void SyncTilemapScenes(World &world, ResourceRegistry &resources) {
	// All mutations happen after query iteration. A prefab may itself contain
	// tilemaps; those join next frame, and ancestry guards bound recursion.
	std::vector<Entity> owners, orphans;
	world.Each<Tilemap>([&](Entity entity, Tilemap &) { owners.push_back(entity); });
	world.Each<TileSceneCell>([&](Entity entity, TileSceneCell &) {
		if (!entity.GetParent().Has<Tilemap>()) {
			orphans.push_back(entity);
		}
	});
	for (Entity entity : orphans) {
		entity.Destroy();
	}
	for (Entity owner : owners) {
		if (!owner.IsAlive()) {
			continue;
		}

		struct Desired {
			TileSceneCell marker;
			i32 order;
			bool visible;
			Vector2 position;
		};

		std::vector<Desired> desired;
		const Tilemap *map = owner.Get<Tilemap>();
		if (!map) {
			continue;
		}
		if (map->ValidCellSize()) {
			for (i32 index = 0; index < static_cast<i32>(map->Layers().size()); ++index) {
				const auto &layer = map->Layers()[index];
				for (const auto &[cell, id] : layer.cells) {
					const auto *tile = map->FindTile(id);
					if (tile && tile->scene) {
						desired.push_back({{layer.id, cell, id, tile->resource}, index,
							layer.visible, map->CellToLocal(cell.x, cell.y)});
					}
				}
			}
		}
		using Key = std::tuple<i32, i32, i32>;
		std::map<Key, Entity> existing;
		for (Entity child : owner.GetChildren()) {
			if (const auto *marker = child.Get<TileSceneCell>()) {
				existing[{marker->layerId, marker->cell.x, marker->cell.y}] = child;
			}
		}
		for (const auto &want : desired) {
			const auto &mark = want.marker;
			const Key key{mark.layerId, mark.cell.x, mark.cell.y};
			Entity container;
			if (const auto it = existing.find(key); it != existing.end()) {
				container = it->second;
				existing.erase(it);
				const auto *old = container.Get<TileSceneCell>();
				if (old->tile != mark.tile || old->resource != mark.resource) {
					container.Destroy();
					container = {};
				}
			}
			if (!container.IsAlive()) {
				bool recursive = false;
				i32 depth = 0;
				for (Entity ancestor = owner; ancestor.IsAlive(); ancestor = ancestor.GetParent()) {
					if (++depth > 128) {
						recursive = true;
						break;
					}
					if (const auto *instance = ancestor.Get<SceneInstance>();
						instance && instance->packed == mark.resource) {
						recursive = true;
						break;
					}
				}
				if (recursive) {
					continue;
				}
				const auto packed = resources.Get<PackedScene>(mark.resource);
				if (!packed) {
					continue;
				}
				container = owner.CreateChild();
				container.SetName("Tile_" + std::to_string(mark.layerId) + "_" +
								  std::to_string(mark.cell.x) + "_" + std::to_string(mark.cell.y));
				container.Add(mark);
				container.Add<Transform2D>();
				container.Add<RenderOrder>();
				// Position before instantiate: physics observers may read it.
				container.Get<Transform2D>()->position = want.position;
				if (!packed->Instantiate(world, resources, container).IsAlive()) {
					container.Destroy();
					continue;
				}
			}
			container.Get<Transform2D>()->position = want.position;
			// Use absolute order to also support a map without RenderOrder.
			auto *order = container.Get<RenderOrder>();
			order->zIndexMode = ZIndexMode::Absolute;
			order->zIndex = ComputeEffectiveZIndex(owner) + want.order;
			order->layerMode = RenderLayerMode::Explicit;
			order->layer = ComputeEffectiveLayer(owner);
			container.SetVisible(want.visible);
		}
		for (auto &[key, entity] : existing) {
			entity.Destroy();
		}
	}
}
} // namespace sowa
