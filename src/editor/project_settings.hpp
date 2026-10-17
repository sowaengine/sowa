#pragma once

#include <string>
#include <vector>

#include "core/project_settings.hpp"

namespace sowa::editor {

// Project Settings edits a complete draft rather than mutating live engine
// state. GlobalStore participates too, so one Save commits the whole file.
bool EditableProjectSettingsEqual(const ProjectSettings &a, const ProjectSettings &b);

// Empty means valid. Validation is shared by the panel and Application so a
// request cannot bypass the same basic size/path checks used by the UI.
std::string ValidateEditableProjectSettings(const ProjectSettings &settings);

// Names of the other actions that also use binding. Sharing is allowed;
// the Input section only warns about it.
std::vector<std::string> ActionsSharingBinding(const std::vector<InputAction> &actions,
	std::size_t actionIndex, const InputBinding &binding);

// base, or base_2, base_3... whichever no action uses yet.
std::string UniqueActionName(const std::vector<InputAction> &actions, const std::string &base);

} // namespace sowa::editor
