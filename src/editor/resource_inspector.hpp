#pragma once

#include <string>
#include <vector>

#include "editor/scene_documents.hpp"
#include "resource/resource_registry.hpp"

namespace sowa {
class FileSystem;
}

namespace sowa::editor {

std::string ResourceLabel(ResourceRegistry &resources, ResourceID resource);
std::vector<ResourceID> SceneResourcesOfType(Scene scene, ResourceRegistry &resources,
	std::string_view type);
ResourceID CreateSceneResource(Scene scene, ResourceRegistry &resources, std::string_view type);
std::string SaveResource(Resource &resource);

struct ResourceSelection {
	ResourceID value;
	bool changed = false;
	bool created = false;
	bool editRequested = false;
};

// Shared by component fields and nested resource fields. The caller records
// reference changes; edits to the resource itself use the scene's undo history.
class ResourceInspector {
  public:
	ResourceSelection Draw(const char *id, ResourceID value, std::string_view type, Scene scene,
		ResourceRegistry &resources, FileSystem *files,
		std::vector<SceneDocumentSaveState> &documents, bool editable);

	void Commit(ResourceRegistry &resources, std::vector<SceneDocumentSaveState> &documents);

  private:
	void drawProperties(Resource &resource, Scene scene, ResourceRegistry &resources,
		FileSystem *files, std::vector<SceneDocumentSaveState> &documents, bool editable);

	std::vector<ResourceID> _ancestors;
	Scene _pendingScene;
	ResourceID _pendingResource;
	u32 _activeWidget = 0;
	std::string _beforeEdit;
};
} // namespace sowa::editor
