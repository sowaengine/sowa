#include "core/filesystem/file_system.hpp"
#include "core/log.hpp"
#include "core/window.hpp"
#include "ecs/world.hpp"
#include "editor/asset_picker.hpp"
#include "editor/editor.hpp"
#include "editor/icons.hpp"
#include "editor/style.hpp"
#include "editor/widgets.hpp"
#include "imgui.h"
#include "resource/resource_registry.hpp"
#include "scene/scene_manager.hpp"

#include <algorithm>

namespace sowa {

void Editor::drawSceneTabs(SceneManager &scenes, ResourceRegistry &resources, bool playing) {
	const std::vector<Scene> openDocuments = scenes.EditorScenes();
	const Scene editorCurrent = scenes.EditorCurrent();
	editor::UpdateSceneDocumentOrder(_sceneTabOrder, openDocuments, editorCurrent);

	std::vector<Scene> documents;
	documents.reserve(_sceneTabOrder.size());
	for (Entity root : _sceneTabOrder) {
		Scene document(root);
		if (document.IsValid()) {
			documents.push_back(document);
		}
	}
	if (documents.empty()) {
		_visibleSceneTab = {};
		return;
	}

	const bool syncSelection = editorCurrent.IsValid() && editorCurrent.Root() != _visibleSceneTab;
	if (syncSelection) {
		if (!playing) {
			editor::ActivateSceneDocument(scenes, editorCurrent);
		}
		bindSelection(scenes);
		_editingScene = playing ? Scene() : scenes.EditorCurrent();
	}
	const auto actions = editor::DrawSceneDocumentTabs(documents, editorCurrent, _visibleSceneTab,
		_sceneSaveStates, playing);
	if (actions.activate.IsValid()) {
		commitPendingSceneEdits(resources);
		editor::ActivateSceneDocument(scenes, actions.activate);
		bindSelection(scenes);
		_editingScene = playing ? Scene() : scenes.EditorCurrent();
	}
	const Scene closeRequest = actions.close;

	if (closeRequest.IsValid()) {
		commitPendingSceneEdits(resources);
		if (editor::IsSceneDocumentUnsaved(_sceneSaveStates, closeRequest)) {
			_sceneCloseRequest = closeRequest;
			_openSceneCloseDialog = true;
		} else {

			if (editor::CloseSceneDocument(scenes, closeRequest, documents)) {
				_visibleSceneTab = {};
				bindSelection(scenes);
				_editingScene = playing ? Scene() : scenes.EditorCurrent();
			}
		}
	}

	if (_openSceneCloseDialog) {
		ImGui::OpenPopup("Unsaved scene");
		_openSceneCloseDialog = false;
	}
	ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing,
		ImVec2(0.5f, 0.5f));
	ImGui::SetNextWindowSize(
		ImVec2(std::min(460.f, ImGui::GetMainViewport()->WorkSize.x - 32.f), 0.f));
	if (const editor::DialogStyle dialogStyle;
		ImGui::BeginPopupModal("Unsaved scene", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::TextWrapped("Save changes to '%s' before closing?",
			_sceneCloseRequest.Name().c_str());
		ImGui::Dummy(ImVec2(0.f, 4.f));
		ImGui::Separator();
		ImGui::Dummy(ImVec2(0.f, 2.f));
		bool closeNow = false;
		ImGui::BeginDisabled(_fileSystem == nullptr || _sceneCloseRequest.SourcePath().empty());
		if (ImGui::Button("Save & Close", ImVec2(120.f, 32.f))) {
			closeNow = editor::SaveSceneDocument(*_fileSystem, resources, _sceneCloseRequest,
				_sceneSaveStates);
		}
		ImGui::EndDisabled();
		ImGui::SameLine();
		if (ImGui::Button("Discard", ImVec2(100.f, 32.f))) {
			closeNow = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(100.f, 32.f))) {
			_sceneCloseRequest = {};
			ImGui::CloseCurrentPopup();
		}
		if (closeNow) {

			if (editor::CloseSceneDocument(scenes, _sceneCloseRequest, documents)) {
				_visibleSceneTab = {};
				bindSelection(scenes);
				_editingScene = playing ? Scene() : scenes.EditorCurrent();
			}
			_sceneCloseRequest = {};
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
}

void Editor::drawCreateMenu(Entity parent, bool enabled, bool submenu) {
	if (submenu && !ImGui::BeginMenu("Create", enabled && editor::CanCreateChild(parent))) {
		return;
	}
	if (!submenu && (!enabled || !editor::CanCreateChild(parent))) {
		return;
	}
	const editor::EntityArchetype types[] = {
		editor::EntityArchetype::Empty,
		editor::EntityArchetype::Node2D,
		editor::EntityArchetype::Sprite,
		editor::EntityArchetype::AnimatedSprite,
		editor::EntityArchetype::Text2D,
		editor::EntityArchetype::NinePatchRect,
		editor::EntityArchetype::Particles2D,
		editor::EntityArchetype::Tilemap,
		editor::EntityArchetype::Camera2D,
		editor::EntityArchetype::Parallax2D,
	};
	for (editor::EntityArchetype type : types) {
		if (ImGui::MenuItem(editor::ArchetypeName(type))) {
			Entity spawned =
				editor::ExecuteCreateEntity(_sceneSaveStates, _editingScene, parent, type);
			if (spawned.IsAlive()) {
				SetSelected(spawned);
				_expandHierarchyEntity = parent;
			}
		}
	}
	if (ImGui::BeginMenu("UI")) {
		const editor::EntityArchetype uiTypes[] = {editor::EntityArchetype::UICanvas,
			editor::EntityArchetype::UIRect, editor::EntityArchetype::UIRow,
			editor::EntityArchetype::UIColumn, editor::EntityArchetype::UIGrid,
			editor::EntityArchetype::UIFlow, editor::EntityArchetype::UIOverlay,
			editor::EntityArchetype::UIPanel, editor::EntityArchetype::UIImage,
			editor::EntityArchetype::UINinePatchRect, editor::EntityArchetype::UILabel,
			editor::EntityArchetype::UIButton, editor::EntityArchetype::UIToggle,
			editor::EntityArchetype::UISlider, editor::EntityArchetype::UIProgressBar,
			editor::EntityArchetype::UIScrollView, editor::EntityArchetype::UIScrollbar,
			editor::EntityArchetype::UITextInput, editor::EntityArchetype::UIList,
			editor::EntityArchetype::UIDropdown, editor::EntityArchetype::UIPopup,
			editor::EntityArchetype::UIModal, editor::EntityArchetype::UITabs,
			editor::EntityArchetype::UITooltip};
		for (auto type : uiTypes) {
			if (ImGui::MenuItem(editor::ArchetypeName(type))) {
				Entity spawned =
					editor::ExecuteCreateEntity(_sceneSaveStates, _editingScene, parent, type);
				if (spawned.IsAlive()) {
					SetSelected(spawned);
					_expandHierarchyEntity = parent;
				}
			}
		}
		ImGui::EndMenu();
	}
	if (submenu) {
		ImGui::EndMenu();
	}
}

void Editor::drawHierarchyNode(Entity entity, ResourceRegistry &resources, bool playing,
	bool defaultOpen) {
	if (!entity.IsAlive()) {
		return;
	}

	const bool locked = IsInstanceLocked(entity);
	const std::vector<Entity> children = entity.GetChildren();
	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth |
							   ImGuiTreeNodeFlags_DrawLinesToNodes;
	if (selection().Contains(entity)) {
		flags |= ImGuiTreeNodeFlags_Selected;
	}
	if (children.empty()) {
		flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
	}
	if (_expandHierarchyEntity == entity) {
		ImGui::SetNextItemOpen(true);
		_expandHierarchyEntity = {};
	} else if (defaultOpen) {
		ImGui::SetNextItemOpen(true, ImGuiCond_Once);
	}

	if (locked) {
		ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
	}

	const std::string name = editor::DisplayName(entity);
	// The row retains native tree IDs and last-item state for drag and context actions.
	const auto appearance = editor::EntityAppearance(entity);
	const std::string nodeId = std::to_string(entity.Raw().id());
	const bool instance = IsInstanceRoot(entity);
	const float trailingWidth =
		instance ? ImGui::GetFrameHeight() + ImGui::GetStyle().ItemInnerSpacing.x : 0.f;
	const bool open = editor::TreeRow(nodeId.c_str(), name.c_str(), appearance.icon,
		locked ? ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled) : appearance.color, flags,
		trailingWidth);
	const ImVec2 rowMin = ImGui::GetItemRectMin(), rowMax = ImGui::GetItemRectMax();

	if (locked) {
		ImGui::PopStyleColor();
	}

	_hierarchyVisible.push_back(entity);
	if (Selected() == entity) {
		ImGui::GetWindowDrawList()->AddLine(ImVec2(rowMin.x, rowMin.y), ImVec2(rowMin.x, rowMax.y),
			ImGui::GetColorU32(ImGuiCol_CheckMark), 3.f);
	}
	if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
		commitPendingSceneEdits(resources);
		const auto &io = ImGui::GetIO();
		_hierarchyClick = (!io.KeyShift && !io.KeyCtrl && !io.KeySuper) ? entity : Entity{};
		if (io.KeyShift) {
			_rangeRequest = std::make_pair(entity, io.KeyCtrl || io.KeySuper);
		} else if ((io.KeyCtrl || io.KeySuper) || !selection().Contains(entity)) {
			selection().Click(entity,
				(io.KeyCtrl || io.KeySuper) ? editor::SelectionMode::Toggle
											: editor::SelectionMode::Replace);
		} else {
			selection().Activate(entity);
		}
	}
	// Delay collapse until release so dragging a selected row keeps its group.
	if (_hierarchyClick == entity && ImGui::IsItemHovered() &&
		ImGui::IsMouseReleased(ImGuiMouseButton_Left) &&
		(ImGui::GetMouseDragDelta(ImGuiMouseButton_Left).x == 0.f &&
			ImGui::GetMouseDragDelta(ImGuiMouseButton_Left).y == 0.f) &&
		!ImGui::GetIO().KeyShift && !ImGui::GetIO().KeyCtrl && !ImGui::GetIO().KeySuper &&
		!ImGui::IsItemToggledOpen()) {
		SetSelected(entity);
	}

	if (!playing && editor::CanDrag(entity)) {
		if (ImGui::BeginDragDropSource()) {
			if (!selection().Contains(entity)) {
				SetSelected(entity);
			}
			const auto &members = selection().Members();
			ImGui::SetDragDropPayload("SOWA_ENTITIES", members.data(),
				members.size() * sizeof(Entity));
			ImGui::TextUnformatted(name.c_str());
			ImGui::EndDragDropSource();
		}
	}

	if (!playing && ImGui::BeginDragDropTarget()) {
		if (const auto *payload = ImGui::AcceptDragDropPayload("SOWA_ENTITIES",
				ImGuiDragDropFlags_AcceptBeforeDelivery)) {
			editor::EntitySelection dragged;
			const auto *entities = static_cast<const Entity *>(payload->Data);
			dragged.Assign(
				std::vector<Entity>(entities, entities + payload->DataSize / sizeof(Entity)));
			if (payload->IsDelivery() &&
				editor::CanReparentSelection(dragged, _editingScene, entity)) {
				commitPendingSceneEdits(resources);
				selection() = dragged;
				editor::ExecuteReparentSelection(_sceneSaveStates, _editingScene, entity);
			}
		}
		ImGui::EndDragDropTarget();
	}

	if (const editor::MenuStyle menuStyle; ImGui::BeginPopupContextItem()) {
		if (ImGui::IsWindowAppearing()) {
			commitPendingSceneEdits(resources);
			if (!selection().Contains(entity)) {
				SetSelected(entity);
			} else {
				selection().Activate(entity);
			}
		}
		drawCreateMenu(entity, !playing);
		if (ImGui::MenuItem("Instantiate Scene", nullptr, false,
				!playing && editor::CanCreateChild(entity))) {
			_sceneInstanceParent = entity;
			_sceneInstanceError.clear();
			_openSceneInstancePicker = true;
		}
		ImGui::Separator();
		drawEntityActions(entity, entity, playing, resources);
		if (ImGui::MenuItem("Delete", nullptr, false, !playing && canMutateSelection(true))) {
			deleteSelection(resources);
		}
		ImGui::EndPopup();
	}

	if (instance) {
		const std::string path = editor::InstanceSourcePath(entity, resources);
		std::string tooltip = "Open source scene: " + path;
		if (playing) {
			tooltip = "Stop playback to open the source scene";
		} else if (path.empty()) {
			tooltip = "Source scene is unavailable";
		}
		if (editor::TreeRowButton(nodeId.c_str(), ICON_FA_FILM, tooltip.c_str(), rowMin, rowMax,
				!playing && !path.empty())) {
			_openInstanceSceneRequested = path;
		}
	}

	if (open && !children.empty()) {
		for (Entity child : children) {
			drawHierarchyNode(child, resources, playing, false);
		}
		ImGui::TreePop();
	}
}

void Editor::drawHierarchy(World &world, SceneManager &scenes, ResourceRegistry &resources,
	bool playing) {
	ImGui::Begin(ICON_FA_SITEMAP "  Hierarchy");
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6.f, 2.f));
	const Scene hierarchyScene = scenes.EditorCurrent();
	Entity createParent = editor::CanCreateChild(Selected()) ? Selected() : hierarchyScene.Root();
	ImGui::BeginDisabled(playing || !editor::CanCreateChild(createParent));
	if (ImGui::Button(ICON_FA_PLUS "  Add node")) {
		ImGui::OpenPopup("##hierarchy_add");
	}
	ImGui::SameLine();
	if (editor::IconActionButton(ICON_FA_LINK, "Instantiate a scene as a child")) {
		_sceneInstanceParent = createParent;
		_sceneInstanceError.clear();
		_openSceneInstancePicker = true;
	}
	ImGui::EndDisabled();
	if (const editor::MenuStyle menuStyle; ImGui::BeginPopup("##hierarchy_add")) {
		drawCreateMenu(createParent, !playing, false);
		ImGui::EndPopup();
	}
	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	handleSelectionShortcuts(resources, createParent);

	if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
		!ImGui::IsAnyItemHovered()) {
		SetSelected({});
	}

	// During play, show runtime scenes and autoloads.
	const std::vector<Scene> shownScenes =
		playing ? scenes.RuntimeScenes() : std::vector<Scene>{hierarchyScene};
	_hierarchyVisible.clear();
	bool anyScene = false;
	for (const Scene scene : shownScenes) {
		if (scene.IsValid()) {
			drawHierarchyNode(scene.Root(), resources, playing, true);
			anyScene = true;
		}
	}
	if (_rangeRequest) {
		selection().Range(_rangeRequest->first, _hierarchyVisible, _rangeRequest->second);
		_rangeRequest.reset();
	}
	if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
		_hierarchyClick = {};
	}
	if (!anyScene) {
		ImGui::TextDisabled("No scene open.");
	}

	if (const editor::MenuStyle menuStyle;
		!playing && ImGui::BeginPopupContextWindow("hierarchy_bg",
						ImGuiPopupFlags_NoOpenOverItems | ImGuiPopupFlags_MouseButtonRight)) {
		Entity parent = Selected();
		if (!editor::CanCreateChild(parent) && hierarchyScene.IsValid()) {
			parent = hierarchyScene.Root();
		}
		drawCreateMenu(parent, true);
		drawEntityActions(Selected(), parent, playing, resources);
		if (ImGui::MenuItem("Instantiate Scene", nullptr, false, editor::CanCreateChild(parent))) {
			_sceneInstanceParent = parent;
			_sceneInstanceError.clear();
			_openSceneInstancePicker = true;
		}
		ImGui::EndPopup();
	}
	drawSceneInstancePicker(world, resources, playing);

	ImGui::PopStyleVar();
	ImGui::End();
}

void Editor::drawSceneInstancePicker(World &world, ResourceRegistry &resources, bool playing) {
	if (_openSceneInstancePicker) {
		ImGui::OpenPopup("##scene_instance_picker");
		_openSceneInstancePicker = false;
	}

	const editor::DialogStyle dialogStyle;
	if (!editor::BeginAssetPickerPopup("##scene_instance_picker")) {
		return;
	}
	const auto picker = editor::DrawAssetPicker(_fileSystem, editor::AssetKind::Scene,
		"Instantiate a scene", _sceneInstanceError);
	if (const auto &path = picker.path; path && _fileSystem) {
		const editor::SceneInstantiationResult result = editor::TryInstantiateScene(
			_sceneInstanceParent, *path, playing, *_fileSystem, world, resources);
		_sceneInstanceError = result.error;
		if (result) {
			editor::RecordInstantiatedScene(_sceneSaveStates, _editingScene, result.instance,
				resources, result.packed, result.ownershipAdded);
			SetSelected(result.instance);
			_expandHierarchyEntity = _sceneInstanceParent;
			_sceneInstanceParent = {};
			ImGui::CloseCurrentPopup();
		}
	}
	if (picker.cancelled) {
		_sceneInstanceParent = {};
		_sceneInstanceError.clear();
		ImGui::CloseCurrentPopup();
	}
	ImGui::EndPopup();
}

void Editor::copyEntity(ResourceRegistry &resources) {
	if (multiSelected()) {
		return;
	}
	commitPendingSceneEdits(resources);
	const auto text = editor::CopyEntity(Selected(), resources);
	if (!text.empty() && _window) {
		_window->SetClipboard(text);
	}
}

void Editor::pasteEntity(Entity parent, ResourceRegistry &resources) {
	if (multiSelected() || !_window || !_editingWorld || !parent.IsAlive()) {
		return;
	}
	commitPendingSceneEdits(resources);
	// All editor entities share the application's World; obtain it at the Draw call.
	if (auto copy = editor::ExecutePasteEntity(_sceneSaveStates, _editingScene, parent,
			_window->GetClipboard(), *_editingWorld, resources);
		copy.IsAlive()) {
		SetSelected(copy);
		_expandHierarchyEntity = parent;
	} else {
		Log::Warn("Could not paste entity. The clipboard may contain unavailable components or "
				  "resources.");
	}
}

void Editor::duplicateEntity(ResourceRegistry &resources) {
	commitPendingSceneEdits(resources);
	if (editor::ExecuteDuplicateSelection(_sceneSaveStates, _editingScene)) {
		_expandHierarchyEntity = Selected().GetParent();
	}
}

void Editor::drawEntityActions(Entity entity, Entity pasteParent, bool playing,
	ResourceRegistry &resources) {
	const bool canCopy = !playing && canMutateSelection(true);
	if (ImGui::MenuItem("Duplicate", "Ctrl/Cmd+D", false, canCopy)) {
		duplicateEntity(resources);
	}
	if (ImGui::MenuItem("Copy", "Ctrl/Cmd+C", false, canCopy && !multiSelected())) {
		copyEntity(resources);
	}
	const bool canPaste = !playing && !multiSelected() && editor::CanCreateChild(pasteParent) &&
		_window && editor::IsEntityClipboard(_window->GetClipboard());
	if (ImGui::MenuItem("Paste as child", "Ctrl/Cmd+V", false, canPaste)) {
		pasteEntity(pasteParent, resources);
	}
}

} // namespace sowa
