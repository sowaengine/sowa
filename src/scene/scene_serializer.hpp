#pragma once

#include <string_view>
#include <unordered_map>
#include <vector>

#include "core/filesystem/fs_types.hpp"
#include "core/serialize/archive.hpp"
#include "resource/resource_registry.hpp"
#include "scene/scene.hpp"

namespace sowa {

class FileSystem;
class World;

// Maps serialized resource IDs to registry IDs after collisions.
using ResourceRemap = std::unordered_map<u64, u64>;

// Loads resources, remapping conflicting IDs. Retain increments their reference counts.
// Owned resources are updated in place; pass the previous remap when reloading.
std::vector<ResourceID> LoadResourceRecords(Archive &archive, ResourceRegistry &resources,
	ResourceRemap &remap, bool retain = false, const std::vector<ResourceID> &owned = {});

// Reload in place, preserving the document root and ownership.
bool ReloadSceneFromFile(FileSystem &fs, Scene scene, World &world, ResourceRegistry &resources);

// Saves nested entities in child order and resolves entity paths after loading.
// Resources load first under saved IDs. Prefabs save instance IDs and root overrides
// without inherited children. Archive errors retain recoverable data.
bool SaveScene(Archive &archive, Scene scene, ResourceRegistry &resources);

// Loads a scene, updating resources already listed in owned. Returns an invalid
// scene if the document has no root or the world cannot create it.
Scene LoadScene(Archive &archive, World &world, ResourceRegistry &resources,
	const std::vector<ResourceID> &owned = {});

FsError SaveSceneToFile(FileSystem &fs, std::string_view path, Scene scene,
	ResourceRegistry &resources);

// Loads and stamps the scene's sourcePath, so it knows where to save back to.
Scene LoadSceneFromFile(FileSystem &fs, std::string_view path, World &world,
	ResourceRegistry &resources);

// Discards any snapshot, releases owned resources, destroys the entity subtree,
// and invalidates scene. Other owners keep shared resources alive.
// Retain a resource through another owner to keep it beyond scene unload.
void UnloadScene(Scene &scene, ResourceRegistry &resources);

} // namespace sowa
