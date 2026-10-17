#pragma once

#include "resource/resource_registry.hpp"
#include "scene/scene.hpp"

namespace sowa {

// Disabled subtree snapshot with saved resource fields and extra retains. Restore
// replaces the live tree and reloads only changed resources. Other scene roots are unaffected.

// False if the scene is invalid, already snapshotted, or the duplicate fails.
bool TakeSnapshot(Scene scene, ResourceRegistry &resources);

// Consumes the snapshot and replaces the live scene root, updating the handle.
// Returns false if no snapshot exists.
bool RestoreSnapshot(Scene &scene, ResourceRegistry &resources);

// Drops the snapshot without restoring. The live scene is left as it is,
// including any mutations made since Take. No-op if there isn't one.
void DiscardSnapshot(Scene scene, ResourceRegistry &resources);

} // namespace sowa
