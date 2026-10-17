#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "core/id_generator.hpp"
#include "editor/command_history.hpp"
#include "scene/scene.hpp"

namespace sowa {

class SceneManager;
class FileSystem;
class ResourceRegistry;

namespace editor {

// Activates an editor document and pauses background documents.
bool ActivateSceneDocument(SceneManager &scenes, Scene scene);

struct SceneDocumentContext {
	Entity root;
	Entity indexedRoot;
	u64 reloadRevision = 0;
	std::string sourcePath;
	Entity graveyard;
	RandomIdGenerator ids;
	u64 documentId = 0;
	std::unordered_map<u64, Entity> entityById;

	~SceneDocumentContext() {
		if (graveyard.IsAlive()) {
			graveyard.Destroy();
		}
	}
};

struct SceneDocumentSaveState {
	std::shared_ptr<SceneDocumentContext> context;
	CommandHistory history;

	explicit SceneDocumentSaveState(bool initiallySaved = true)
		: context(std::make_shared<SceneDocumentContext>()), history(initiallySaved) {
	}

	SceneDocumentSaveState(SceneDocumentSaveState &&) noexcept = default;
	SceneDocumentSaveState &operator=(SceneDocumentSaveState &&) noexcept = default;
};

// Update histories for the live editor documents. This does no scene
// serialization and no content comparison; it only updates document handles
// after open/close and play-mode snapshot restoration.
void UpdateSceneDocumentStates(std::vector<SceneDocumentSaveState> &states,
	const std::vector<Scene> &documents);

// Ensures an editable entity has a non-zero id unique within its document.
// Prefab-locked descendants are not authored by the host document and
// therefore do not receive host ids.
u64 EnsureEditorEntityId(SceneDocumentSaveState &state, Entity entity);

SceneDocumentSaveState *FindSceneDocumentState(std::vector<SceneDocumentSaveState> &states,
	Scene scene);
const SceneDocumentSaveState *FindSceneDocumentState(
	const std::vector<SceneDocumentSaveState> &states, Scene scene);

bool IsSceneDocumentUnsaved(const std::vector<SceneDocumentSaveState> &states, Scene scene);

// Save to Scene::SourcePath, checkpoint the command history, and write success
// or failure to the shared Console log.
bool SaveSceneDocument(FileSystem &fs, ResourceRegistry &resources, Scene scene,
	std::vector<SceneDocumentSaveState> &states);

bool UndoSceneDocument(std::vector<SceneDocumentSaveState> &states, Scene scene);
bool RedoSceneDocument(std::vector<SceneDocumentSaveState> &states, Scene scene);

// Update a persistent tab order from the currently open documents. Existing
// tabs retain their positions; unseen documents are appended, with a newly
// current document appended after any other documents discovered in that frame.
void UpdateSceneDocumentOrder(std::vector<Entity> &order, const std::vector<Scene> &documents,
	Scene current);

struct SceneDocumentTabActions {
	Scene activate;
	Scene close;
};

// Track the tab ImGui actually displays, since SetSelected takes effect next frame.
SceneDocumentTabActions DrawSceneDocumentTabs(const std::vector<Scene> &documents, Scene current,
	Entity &visibleRoot, const std::vector<SceneDocumentSaveState> &states, bool playing);

// Queue a document close. Closing the active document first activates the next
// document, or the previous one when the active document is last.
bool CloseSceneDocument(SceneManager &scenes, Scene scene,
	const std::vector<Scene> &orderedDocuments);

} // namespace editor
} // namespace sowa
