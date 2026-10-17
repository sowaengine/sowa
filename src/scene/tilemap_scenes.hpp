#pragma once
#include "ecs/components/tilemap.hpp"
#include "ecs/entity.hpp"

namespace sowa {
class World;
class ResourceRegistry;

// Derived cell container. Kept after its prefab is consumed, so a coin killed
// by gameplay does not respawn every frame. Not registered for serialization.
struct TileSceneCell {
	i32 layerId = 0;
	TileCell cell;
	i32 tile = 0;
	ResourceID resource;
};

// Call outside iteration, before gameplay/transforms. Renderer Init installs
// this in PreUpdate; headless clients can call it explicitly.
void SyncTilemapScenes(World &world, ResourceRegistry &resources);
Entity GetTileSceneEntity(Entity owner, i32 layer, i32 x, i32 y);
bool IsGeneratedTileEntity(Entity entity);
} // namespace sowa
