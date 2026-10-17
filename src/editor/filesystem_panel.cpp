#include "core/filesystem/file_system.hpp"
#include "core/log.hpp"
#include "ecs/world.hpp"
#include "editor/editor.hpp"
#include "editor/icons.hpp"
#include "editor/style.hpp"
#include "editor/widgets.hpp"
#include "imgui.h"
#include "scene/scene_manager.hpp"

#include <algorithm>
#include <cfloat>
#include <cstdio>

namespace sowa {
namespace {

editor::TypeAppearance assetAppearance(editor::AssetKind kind) {
	using namespace editor::theme;
	switch (kind) {
	case editor::AssetKind::Directory:
		return {ICON_FA_FOLDER, Blue, "Folder"};
	case editor::AssetKind::Scene:
		return {ICON_FA_FILM, Blue, "Scene"};
	case editor::AssetKind::Script:
		return {ICON_FA_FILE_CODE, Green, "Script"};
	case editor::AssetKind::Image:
		return {ICON_FA_IMAGE, Blue, "Image"};
	case editor::AssetKind::Audio:
		return {ICON_FA_MUSIC, Amber, "Audio"};
	case editor::AssetKind::Font:
		return {ICON_FA_FONT, Rose, "Font"};
	default:
		return {ICON_FA_FILE, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled), "File"};
	}
}

} // namespace

void Editor::drawFilesystemContext(SceneManager &scenes, const std::string &path,
	editor::AssetKind kind, bool playing) {
	const editor::MenuStyle menuStyle;
	if (!ImGui::BeginPopupContextItem()) {
		return;
	}
	_selectedAsset = path;
	if (kind == editor::AssetKind::Scene) {
		if (ImGui::MenuItem("Open", nullptr, false, !playing)) {
			editor::TryOpenAsset(scenes, path, kind);
		}
	}
	if (kind == editor::AssetKind::Directory) {
		drawFilesystemCreateMenu(path, playing);
	}
	if (ImGui::MenuItem("Copy Path")) {
		ImGui::SetClipboardText(path.c_str());
	}
	if (path != "res://") {
		ImGui::Separator();
		if (ImGui::MenuItem("Rename", nullptr, false, !playing)) {
			requestFilesystemDialog(FilesystemDialogAction::Rename, path, kind);
		}
		if (ImGui::MenuItem("Delete", nullptr, false, !playing)) {
			requestFilesystemDialog(FilesystemDialogAction::Delete, path, kind);
		}
	}
	ImGui::EndPopup();
}

void Editor::drawFilesystemCreateMenu(const std::string &directory, bool playing) {
	if (!ImGui::BeginMenu("Create", !playing)) {
		return;
	}
	if (ImGui::MenuItem(ICON_FA_FILM "  Scene...")) {
		requestFilesystemDialog(FilesystemDialogAction::CreateScene, directory);
	}
	if (ImGui::MenuItem(ICON_FA_FILE_CODE "  Script...")) {
		requestFilesystemDialog(FilesystemDialogAction::CreateScript, directory);
	}
	ImGui::Separator();
	if (ImGui::MenuItem(ICON_FA_FOLDER "  Directory...")) {
		requestFilesystemDialog(FilesystemDialogAction::CreateDirectory, directory);
	}
	if (ImGui::MenuItem(ICON_FA_FILE "  File...")) {
		requestFilesystemDialog(FilesystemDialogAction::CreateFile, directory);
	}
	ImGui::EndMenu();
}

void Editor::requestFilesystemDialog(FilesystemDialogAction action, const std::string &path,
	editor::AssetKind kind) {
	_filesystemDialogAction = action;
	_filesystemDialogPath = path;
	_filesystemDialogAssetKind = kind;
	_filesystemDialogError.clear();
	_filesystemDialogName[0] = '\0';
	const char *initialName = action == FilesystemDialogAction::CreateScene		  ? "NewScene.scene"
							  : action == FilesystemDialogAction::CreateScript	  ? "NewScript.lua"
							  : action == FilesystemDialogAction::CreateDirectory ? "NewDirectory"
							  : action == FilesystemDialogAction::CreateFile	  ? "NewFile"
																				  : "";
	std::snprintf(_filesystemDialogName, sizeof(_filesystemDialogName), "%s", initialName);
	if (action == FilesystemDialogAction::Rename) {
		std::snprintf(_filesystemDialogName, sizeof(_filesystemDialogName), "%s",
			editor::AssetName(path).c_str());
	}
	_openFilesystemDialog = true;
}

void Editor::drawFilesystemDialog(SceneManager &scenes, bool playing) {
	const bool createScene = _filesystemDialogAction == FilesystemDialogAction::CreateScene;
	const bool createScript = _filesystemDialogAction == FilesystemDialogAction::CreateScript;
	const bool createFile = _filesystemDialogAction == FilesystemDialogAction::CreateFile;
	const bool createDirectory = _filesystemDialogAction == FilesystemDialogAction::CreateDirectory;
	const bool creating = createScene || createScript || createFile || createDirectory;
	const bool rename = _filesystemDialogAction == FilesystemDialogAction::Rename;
	const bool remove = _filesystemDialogAction == FilesystemDialogAction::Delete;
	const char *verb = creating ? "Create" : rename ? "Rename" : "Delete";
	const char *title = createScene		  ? "Create scene###FilesystemAction"
						: createScript	  ? "Create script###FilesystemAction"
						: createFile	  ? "Create file###FilesystemAction"
						: createDirectory ? "Create directory###FilesystemAction"
						: rename		  ? "Rename asset###FilesystemAction"
										  : "Delete asset###FilesystemAction";

	if (_openFilesystemDialog) {
		ImGui::OpenPopup(title);
		_openFilesystemDialog = false;
	}
	const editor::DialogStyle dialogStyle;
	const auto *viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	ImGui::SetNextWindowSize(ImVec2(std::min(460.f, viewport->WorkSize.x - 32.f), 0.f));
	if (!ImGui::BeginPopupModal(title, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
		return;
	}

	if (remove) {
		ImGui::TextWrapped(_filesystemDialogAssetKind == editor::AssetKind::Directory
							   ? "Delete this directory and all of its contents?"
							   : "Delete this file?");
	} else if (createScene) {
		ImGui::TextWrapped("Create a scene with a 2D root node.");
	} else if (createScript) {
		ImGui::TextWrapped("Create a Lua component with start and update methods.");
	}
	ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
	ImGui::TextWrapped("%s", _filesystemDialogPath.c_str());
	ImGui::PopStyleColor();
	ImGui::Dummy(ImVec2(0.f, 2.f));

	bool submit = false;
	if (!remove) {
		ImGui::TextUnformatted("Name");
		ImGui::BeginDisabled(playing);
		if (ImGui::IsWindowAppearing()) {
			ImGui::SetKeyboardFocusHere();
		}
		ImGui::SetNextItemWidth(-FLT_MIN);
		submit =
			ImGui::InputText("##asset_name", _filesystemDialogName, sizeof(_filesystemDialogName),
				ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue);
		ImGui::EndDisabled();
	}

	if (!_filesystemDialogError.empty()) {
		ImGui::TextColored(editor::theme::Rose, "%s", _filesystemDialogError.c_str());
	}
	if (playing) {
		ImGui::TextDisabled("Stop the game to modify project files.");
	}

	ImGui::Dummy(ImVec2(0.f, 4.f));
	ImGui::Separator();
	ImGui::Dummy(ImVec2(0.f, 2.f));
	const float buttonWidth = std::min(110.f, (ImGui::GetContentRegionAvail().x - 10.f) * 0.5f);
	ImGui::SetCursorPosX(ImGui::GetCursorPosX() + std::max(0.f, ImGui::GetContentRegionAvail().x -
																	buttonWidth * 2.f - 10.f));
	bool close =
		ImGui::Button("Cancel", ImVec2(buttonWidth, 32.f)) || ImGui::IsKeyPressed(ImGuiKey_Escape);
	ImGui::SameLine();
	const bool canSubmit = !playing && (remove || editor::IsValidAssetName(_filesystemDialogName));
	ImGui::BeginDisabled(!canSubmit);
	ImGui::PushStyleColor(ImGuiCol_Button, remove
											   ? ImVec4(0.58f, 0.16f, 0.14f, 1.f)
											   : ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
	submit = (ImGui::Button(verb, ImVec2(buttonWidth, 32.f)) || submit) && canSubmit && !close;
	ImGui::PopStyleColor();
	ImGui::EndDisabled();

	if (submit && _fileSystem != nullptr) {
		FsError error = FsError::InvalidPath;
		if (creating) {
			const auto result =
				editor::CreateAsset(*_fileSystem, _filesystemDialogPath, _filesystemDialogName,
					createScene		  ? editor::AssetCreateKind::Scene
					: createScript	  ? editor::AssetCreateKind::Script
					: createDirectory ? editor::AssetCreateKind::Directory
									  : editor::AssetCreateKind::File);
			error = result.error;
			if (result) {
				_selectedAsset = result.value;
				close = true;
			}
		} else if (rename) {
			const std::string oldPath = _filesystemDialogPath;
			const auto result = editor::RenameAsset(*_fileSystem, oldPath, _filesystemDialogName);
			error = result.error;
			if (result) {
				_selectedAsset = result.value;
				for (Scene scene : scenes.Loaded()) {
					const std::string source = scene.SourcePath();
					if (source == oldPath) {
						scene.SetSourcePath(result.value);
					} else if (_filesystemDialogAssetKind == editor::AssetKind::Directory &&
							   source.starts_with(oldPath + "/")) {
						scene.SetSourcePath(result.value + source.substr(oldPath.size()));
					}
				}
				close = true;
			}
		} else if (remove) {
			error = editor::DeleteAsset(*_fileSystem, _filesystemDialogPath);
			if (error == FsError::Ok) {
				if (_selectedAsset == _filesystemDialogPath ||
					_selectedAsset.starts_with(_filesystemDialogPath + "/")) {
					_selectedAsset.clear();
				}
				close = true;
			}
		}
		if (!close) {
			_filesystemDialogError = FsErrorToString(error);
		}
	}

	if (close) {
		_filesystemDialogAction = FilesystemDialogAction::None;
		_filesystemDialogError.clear();
		ImGui::CloseCurrentPopup();
	}
	ImGui::EndPopup();
}

void Editor::drawFilesystemDir(FileSystem &fs, SceneManager &scenes, const std::string &dirPath,
	bool playing, bool defaultOpen) {
	const std::vector<FileEntry> entries = editor::ListAssets(fs, dirPath);
	if (entries.empty() && defaultOpen) {
		ImGui::TextDisabled("No files in res://");
		return;
	}

	for (const FileEntry &entry : entries) {
		const std::string child = editor::JoinVirtualPath(dirPath, entry.name);
		const editor::AssetKind kind = editor::KindFromName(entry.name, entry.isDirectory);
		if (entry.isDirectory) {
			if (defaultOpen) {
				ImGui::SetNextItemOpen(true, ImGuiCond_Once);
			}
			ImGuiTreeNodeFlags flags =
				ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick |
				ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_DrawLinesToNodes;
			if (_selectedAsset == child) {
				flags |= ImGuiTreeNodeFlags_Selected;
			}
			const bool open = editor::TreeRow(child.c_str(), entry.name.c_str(), ICON_FA_FOLDER,
				editor::theme::Blue, flags);
			if (ImGui::IsItemClicked()) {
				_selectedAsset = child;
			}
			drawFilesystemContext(scenes, child, kind, playing);
			if (open) {
				drawFilesystemDir(fs, scenes, child, playing, false);
				ImGui::TreePop();
			}
		} else {
			ImGuiTreeNodeFlags flags =
				ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen |
				ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_DrawLinesToNodes;
			if (_selectedAsset == child) {
				flags |= ImGuiTreeNodeFlags_Selected;
			}
			const auto appearance = assetAppearance(kind);
			editor::TreeRow(child.c_str(), entry.name.c_str(), appearance.icon, appearance.color,
				flags);
			if (ImGui::IsItemClicked()) {
				_selectedAsset = child;
				if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
					editor::TryOpenAsset(scenes, child, kind);
				}
			}
			drawFilesystemContext(scenes, child, kind, playing);
		}
	}
}

void Editor::drawFilesystem(SceneManager &scenes, bool playing) {
	ImGui::Begin(ICON_FA_FOLDER "  Filesystem");
	ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 20.f);
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6.f, 1.f));
	if (_fileSystem == nullptr) {
		ImGui::TextDisabled("Filesystem is not mounted.");
	} else {
		ImGui::SetNextItemOpen(true, ImGuiCond_Once);
		ImGuiTreeNodeFlags flags =
			ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_DrawLinesToNodes;
		if (_selectedAsset == "res://") {
			flags |= ImGuiTreeNodeFlags_Selected;
		}
		const bool open =
			editor::TreeRow("res://", "res://", ICON_FA_FOLDER_OPEN, editor::theme::Blue, flags);
		if (ImGui::IsItemClicked()) {
			_selectedAsset = "res://";
		}
		drawFilesystemContext(scenes, "res://", editor::AssetKind::Directory, playing);
		if (open) {
			drawFilesystemDir(*_fileSystem, scenes, "res://", playing, true);
			ImGui::TreePop();
		}
		if (const editor::MenuStyle menuStyle;
			ImGui::BeginPopupContextWindow("filesystem_background",
				ImGuiPopupFlags_NoOpenOverItems | ImGuiPopupFlags_MouseButtonRight)) {
			drawFilesystemCreateMenu("res://", playing);
			ImGui::Separator();
			if (ImGui::MenuItem("Copy Root Path")) {
				ImGui::SetClipboardText("res://");
			}
			ImGui::EndPopup();
		}
		drawFilesystemDialog(scenes, playing);
	}
	ImGui::PopStyleVar(2);
	ImGui::End();
}

} // namespace sowa
