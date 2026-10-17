#pragma once

#include <string>
#include <vector>

#include "ecs/components/scene_root.hpp"
#include "ecs/entity.hpp"
#include "resource/resource_id.hpp"

namespace sowa {

class ResourceRegistry;
class World;

// Copyable, non-owning scene handle. World owns its root and subtree; the handle
// becomes invalid when the root dies and must not outlive World.
class Scene {
  public:
	Scene() = default;

	explicit Scene(Entity root) : _root(root) {
	}

	// Valid while the root is alive and carries SceneRoot.
	bool IsValid() const;

	explicit operator bool() const {
		return IsValid();
	}

	Entity Root() const {
		return _root;
	}

	// Root name and first path segment within the scene.
	std::string Name() const;

	std::string SourcePath() const;
	void SetSourcePath(const std::string &path);

	// Rendering camera, or null if unset or destroyed.
	Entity ActiveCamera() const;

	// Rejects cameras outside this scene.
	bool SetActiveCamera(Entity camera);

	// Creates a child of the scene root.
	Entity CreateEntity(const std::string &name = "");

	// Enabled state affects the whole subtree: flecs does not inherit EcsDisabled, so
	// SetEnabled walks every entity. Independent of current scene and render visibility.

	// Walks every entity in the scene. No-op on an invalid scene.
	void SetEnabled(bool enabled);

	// The root's own flag. False for an invalid scene. A child disabled on
	// its own does not change this.
	bool IsEnabled() const;

	// Owned resource records belong in this scene's file. Own/Disown also update registry
	// owner counts. Dependencies retained by a referenced prefab remain in that prefab's
	// records, not this list.

	// Retains and tracks a registered resource ID. Rejects invalid or already owned IDs.
	bool Own(ResourceRegistry &registry, ResourceID id);

	// Stops tracking and releases. The resource survives if anything else
	// still owns it.
	bool Disown(ResourceRegistry &registry, ResourceID id);

	bool Owns(ResourceID id) const;

	// In the order they were taken on, which is the order they're written in.
	std::vector<ResourceID> OwnedResources() const;

	// Unload with UnloadScene to release owned resources as well as entities.
	// Retain through another owner before Disown or unload to keep a resource alive.

  private:
	Entity _root;
};

// Creates a top-level scene root with a unique name, such as Level or Level2.
Scene CreateScene(World &world, const std::string &name);

// The scene an entity belongs to - the nearest ancestor carrying SceneRoot,
// or the entity itself if it is one. Invalid for an entity outside any scene.
Scene SceneOf(Entity entity);

// Snapshot marker excluded from registered components and GetScenes. Removed when
// restoration makes the copy live. Scene::SetEnabled controls ordinary scene pausing.
struct SceneSnapshotCopy {};

// Prefab instance marker, saved as instance: <id> rather than a component.
// The serializer writes root overrides without baking inherited children.
struct SceneInstance {
	ResourceID packed;
	// Full inherited root fields before host edits, for three-way hot reload.
	std::string defaults;
	u64 revision = 0;
};

// True if this entity is the root of a PackedScene instance.
bool IsInstanceRoot(Entity entity);

// True if this entity sits under an instance root and is not one itself -
// the children the editor should treat as locked. Nested instance roots
// (an enemy that contains a weapon instance) are not locked.
bool IsInstanceLocked(Entity entity);

// Loaded top-level scenes in creation order, including disabled ones.
// Excludes nested scene roots and snapshots.
std::vector<Scene> GetScenes(World &world);

// By root name; invalid if there's no such scene.
Scene FindScene(World &world, const std::string &name);

} // namespace sowa
