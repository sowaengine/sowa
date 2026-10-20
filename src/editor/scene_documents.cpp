#include "editor/scene_documents.hpp"

#include <algorithm>
#include <cstddef>
#include <utility>
#include <vector>

#include "imgui.h"

#include "core/filesystem/file_system.hpp"
#include "core/log.hpp"
#include "ecs/components/editor_entity_id.hpp"
#include "editor/hierarchy.hpp"
#include "resource/resource_registry.hpp"
#include "scene/scene_manager.hpp"
#include "scene/scene_serializer.hpp"

namespace sowa::editor {
namespace {

bool matches(const SceneDocumentSaveState &state, Scene scene) {
	if (!state.context || !scene.IsValid()) {
		return false;
	}
	if (state.context->root == scene.Root()) {
		return true;
	}
	if (const EditorEntityId *id = scene.Root().Get<EditorEntityId>();
		id != nullptr && id->Value() != 0 && state.context->documentId == id->Value()) {
		return true;
	}
	return false;
}

void ensureTreeIds(SceneDocumentSaveState &state, Entity entity) {
	if (!entity.IsAlive() || IsInstanceLocked(entity)) {
		return;
	}
	EnsureEditorEntityId(state, entity);
	for (Entity child : entity.GetChildren()) {
		ensureTreeIds(state, child);
	}
}

} // namespace

bool ActivateSceneDocument(SceneManager &scenes, Scene scene) {
	if (scenes.IsPlaying() || !scene.IsValid() || !scenes.SetCurrent(scene)) {
		return false;
	}

	for (Scene document : scenes.EditorScenes()) {
		document.SetEnabled(document.Root() == scene.Root());
	}
	return true;
}

void UpdateSceneDocumentStates(std::vector<SceneDocumentSaveState> &states,
	const std::vector<Scene> &documents) {
	std::vector<SceneDocumentSaveState> refreshed;
	std::vector<bool> reused(states.size(), false);
	refreshed.reserve(documents.size());

	for (Scene document : documents) {
		if (!document.IsValid()) {
			continue;
		}
		const std::string sourcePath = document.SourcePath();
		std::size_t match = states.size();
		for (std::size_t i = 0; i < states.size(); ++i) {
			if (!reused[i] && states[i].context && states[i].context->root == document.Root()) {
				match = i;
				break;
			}
		}
		if (match == states.size()) {
			const EditorEntityId *documentId = document.Root().Get<EditorEntityId>();
			if (documentId != nullptr && documentId->Value() != 0) {
				for (std::size_t i = 0; i < states.size(); ++i) {
					if (!reused[i] && states[i].context &&
						states[i].context->documentId == documentId->Value()) {
						match = i;
						break;
					}
				}
			}
		}
		if (match != states.size()) {
			reused[match] = true;
			SceneDocumentSaveState state = std::move(states[match]);
			state.context->root = document.Root();
			state.context->sourcePath = sourcePath;
			refreshed.push_back(std::move(state));
		} else {
			SceneDocumentSaveState state(!sourcePath.empty());
			state.context->root = document.Root();
			state.context->sourcePath = sourcePath;
			refreshed.push_back(std::move(state));
		}
	}

	states = std::move(refreshed);
	for (SceneDocumentSaveState &state : states) {
		const auto *root = state.context->root.Get<SceneRoot>();
		if (root && root->reloadRevision != state.context->reloadRevision) {
			state.history.Clear();
			state.history.MarkSaved();
			state.context->reloadRevision = root->reloadRevision;
			state.context->indexedRoot = Entity();
		}
		if (state.context->indexedRoot == state.context->root) {
			state.context->selection.Prune(state.context->root);
			state.context->selectionBookmark =
				SelectionBookmark::Capture(state.context->root, state.context->selection);
			continue;
		}
		state.context->entityById.clear();
		ensureTreeIds(state, state.context->root);
		state.context->indexedRoot = state.context->root;
		state.context->selection = state.context->selectionBookmark.Restore(state.context->root,
			state.context->entityById);
		if (const EditorEntityId *id = state.context->root.Get<EditorEntityId>(); id != nullptr) {
			state.context->documentId = id->Value();
		}
	}
}

u64 EnsureEditorEntityId(SceneDocumentSaveState &state, Entity entity) {
	if (!state.context || !entity.IsAlive() || IsInstanceLocked(entity)) {
		return 0;
	}
	EditorEntityId *id = entity.Get<EditorEntityId>();
	if (id == nullptr) {
		entity.Add(EditorEntityId{});
		id = entity.Get<EditorEntityId>();
	}
	if (id == nullptr) {
		return 0;
	}
	const auto existing = state.context->entityById.find(id->Value());
	if (id->Value() == 0 ||
		(existing != state.context->entityById.end() && existing->second != entity)) {
		u64 value = 0;
		do {
			value = state.context->ids.NextU64();
		} while (value == 0 || state.context->entityById.contains(value));
		id->Set(value);
	}
	state.context->entityById[id->Value()] = entity;
	return id->Value();
}

SceneDocumentSaveState *FindSceneDocumentState(std::vector<SceneDocumentSaveState> &states,
	Scene scene) {
	auto found = std::find_if(states.begin(), states.end(),
		[&](const SceneDocumentSaveState &state) { return matches(state, scene); });
	return found == states.end() ? nullptr : &*found;
}

const SceneDocumentSaveState *FindSceneDocumentState(
	const std::vector<SceneDocumentSaveState> &states, Scene scene) {
	auto found = std::find_if(states.begin(), states.end(),
		[&](const SceneDocumentSaveState &state) { return matches(state, scene); });
	return found == states.end() ? nullptr : &*found;
}

bool IsSceneDocumentUnsaved(const std::vector<SceneDocumentSaveState> &states, Scene scene) {
	const SceneDocumentSaveState *state = FindSceneDocumentState(states, scene);
	return state != nullptr && state->history.IsDirty();
}

bool SaveSceneDocument(FileSystem &fs, ResourceRegistry &resources, Scene scene,
	std::vector<SceneDocumentSaveState> &states) {
	if (!scene.IsValid()) {
		Log::Error("Cannot save scene: no scene is active");
		return false;
	}

	const std::string path = scene.SourcePath();
	if (path.empty()) {
		Log::Error("Cannot save scene '{}': it has no source path", scene.Name());
		return false;
	}

	const FsError error = SaveSceneToFile(fs, path, scene, resources);
	if (error != FsError::Ok) {
		Log::Error("Failed to save scene '{}' to '{}': {}", scene.Name(), path,
			FsErrorToString(error));
		return false;
	}

	SceneDocumentSaveState *state = FindSceneDocumentState(states, scene);
	if (state == nullptr) {
		states.emplace_back(true);
		states.back().context->root = scene.Root();
		states.back().context->sourcePath = path;
	} else {
		state->context->root = scene.Root();
		state->context->sourcePath = path;
		state->history.MarkSaved();
	}

	Log::Info("Saved scene '{}' to '{}'", scene.Name(), path);
	return true;
}

bool UndoSceneDocument(std::vector<SceneDocumentSaveState> &states, Scene scene) {
	SceneDocumentSaveState *state = FindSceneDocumentState(states, scene);
	return state != nullptr && state->history.Undo();
}

bool RedoSceneDocument(std::vector<SceneDocumentSaveState> &states, Scene scene) {
	SceneDocumentSaveState *state = FindSceneDocumentState(states, scene);
	return state != nullptr && state->history.Redo();
}

void UpdateSceneDocumentOrder(std::vector<Entity> &order, const std::vector<Scene> &documents,
	Scene current) {
	auto isOpen = [&](Entity root) {
		return std::any_of(documents.begin(), documents.end(),
			[&](Scene document) { return document.IsValid() && document.Root() == root; });
	};
	auto appendIfMissing = [&](std::vector<Entity> &next, Scene document) {
		if (!document.IsValid() ||
			std::find(next.begin(), next.end(), document.Root()) != next.end()) {
			return;
		}
		next.push_back(document.Root());
	};

	std::vector<Entity> next;
	next.reserve(documents.size());
	for (Entity root : order) {
		if (isOpen(root) && std::find(next.begin(), next.end(), root) == next.end()) {
			next.push_back(root);
		}
	}

	for (Scene document : documents) {
		if (current.IsValid() && document.Root() == current.Root()) {
			continue;
		}
		appendIfMissing(next, document);
	}
	if (current.IsValid() && isOpen(current.Root())) {
		appendIfMissing(next, current);
	}
	order = std::move(next);
}

SceneDocumentTabActions DrawSceneDocumentTabs(const std::vector<Scene> &documents, Scene current,
	Entity &visibleRoot, const std::vector<SceneDocumentSaveState> &states, bool playing) {
	SceneDocumentTabActions actions;
	const bool syncSelection = current.IsValid() && current.Root() != visibleRoot;
	ImGui::BeginDisabled(playing);
	if (ImGui::BeginTabBar("##scene_documents", ImGuiTabBarFlags_FittingPolicyScroll)) {
		for (Scene document : documents) {
			ImGui::PushID(
				reinterpret_cast<void *>(static_cast<uintptr_t>(document.Root().Raw().id())));
			const bool unsaved = IsSceneDocumentUnsaved(states, document);
			const std::string label = document.Name() + (unsaved ? " *" : "") + "###scene_document";
			ImGuiTabItemFlags flags = ImGuiTabItemFlags_None;
			if (syncSelection && document.Root() == current.Root()) {
				flags = ImGuiTabItemFlags_SetSelected;
			}
			bool open = true;
			const bool visible =
				ImGui::BeginTabItem(label.c_str(), playing ? nullptr : &open, flags);
			const std::string sourcePath = document.SourcePath();
			if (ImGui::IsItemHovered() && !sourcePath.empty()) {
				ImGui::SetTooltip("%s", sourcePath.c_str());
			}
			if (visible) {
				visibleRoot = document.Root();
				// During an external switch, the old visible tab is not a user selection.
				if (!playing && !syncSelection && document.Root() != current.Root()) {
					actions.activate = document;
				}
				ImGui::EndTabItem();
			}
			if (!open) {
				actions.close = document;
			}
			ImGui::PopID();
		}
		ImGui::EndTabBar();
	}
	ImGui::EndDisabled();
	return actions;
}

bool CloseSceneDocument(SceneManager &scenes, Scene scene,
	const std::vector<Scene> &orderedDocuments) {
	if (scenes.IsPlaying() || !scene.IsValid()) {
		return false;
	}

	std::size_t index = orderedDocuments.size();
	for (std::size_t i = 0; i < orderedDocuments.size(); ++i) {
		if (orderedDocuments[i].Root() == scene.Root()) {
			index = i;
			break;
		}
	}
	if (index == orderedDocuments.size()) {
		return false;
	}

	const Scene current = scenes.EditorCurrent();
	if (current.IsValid() && current.Root() == scene.Root()) {
		Scene successor;
		if (index + 1 < orderedDocuments.size()) {
			successor = orderedDocuments[index + 1];
		} else if (index > 0) {
			successor = orderedDocuments[index - 1];
		}

		if (successor.IsValid()) {
			ActivateSceneDocument(scenes, successor);
		} else {
			scenes.SetCurrent(Scene());
		}
	}

	scenes.Close(scene);
	return true;
}

} // namespace sowa::editor
