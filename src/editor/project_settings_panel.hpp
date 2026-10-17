#pragma once

#include <optional>
#include <string>
#include <vector>

#include "core/input.hpp"
#include "core/project_settings.hpp"
#include "editor/binding_capture.hpp"

namespace sowa::editor {

class ProjectSettingsPanel {
  public:
	ProjectSettingsPanel() = default;
	~ProjectSettingsPanel();
	ProjectSettingsPanel(const ProjectSettingsPanel &) = delete;
	ProjectSettingsPanel &operator=(const ProjectSettingsPanel &) = delete;

	void Init(Input *input);
	void Shutdown();
	void Open();
	// Suppress shortcuts while recording a binding. Call after ImGui::NewFrame.
	void BeginFrame();
	void Draw(ProjectSettings &project, FileSystem &files, bool playing);
	bool HasUnsavedChanges(const ProjectSettings &project) const;
	std::optional<ProjectSettings> TakeSaveRequest();
	void ReportSaveResult(bool success, std::string message);

  private:
	enum class AssetPickerTarget {
		None,
		MainScene,
		Icon,
		Script,
		Autoload
	};
	void drawPathList(const char *id, std::vector<std::string> &paths, bool scripts);
	void drawAssetPicker(FileSystem &files, bool playing);
	void drawInputActions();
	void beginBindingCapture(std::size_t action);
	void finishBindingCapture();

	bool _openRequested = false;
	ProjectSettings _draft;
	ProjectSettings _source;
	bool _draftInitialized = false;
	std::optional<ProjectSettings> _saveRequest;
	std::string _saveStatus;
	bool _saveFailed = false;
	AssetPickerTarget _assetPickerTarget = AssetPickerTarget::None;
	std::optional<std::size_t> _assetPickerIndex;
	bool _openAssetPicker = false;
	std::string _assetPickerError;
	std::string _newStoreKey;
	std::string _newStoreValue;
	Input *_input = nullptr;
	Input::EventCallbackList::Handle _bindingCaptureHandle;
	BindingCapture _bindingCapture;
	std::size_t _bindingCaptureAction = 0;
};

} // namespace sowa::editor
