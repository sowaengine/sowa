#pragma once

#include <string>

#include "ecs/entity.hpp"

namespace sowa {

// Scene metadata on its root entity. A scene is a root and its subtree in the shared
// World. Destroying the root destroys its children. A root without Transform2D is
// transparent to transform inheritance; adding one moves the entire scene.
struct SceneRoot {
	// Where this scene was loaded from, for saving back and reloading. Empty
	// for a scene built in memory.
	std::string sourcePath;
	// Last loaded/saved disk bytes; editor-only metadata, not reflected.
	std::string diskContent;
	u64 reloadRevision = 0;

	// This scene's active camera. Serialized as an entity path and resolved by the scene loader.
	Entity activeCamera;
};

} // namespace sowa
