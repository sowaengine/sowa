#pragma once

#include <optional>
#include <string>

#include "editor/filesystem_browser.hpp"

namespace sowa::editor {

struct AssetPickerResult {
	std::optional<std::string> path;
	bool cancelled = false;
};

// Call with DialogStyle in scope, and EndPopup when this returns true.
bool BeginAssetPickerPopup(const char *id);
// Shared body for resource paths, script attachment and scene selection.
// Other accepts any file; directories are navigation-only.
AssetPickerResult DrawAssetPicker(FileSystem *files, AssetKind kind, const char *title,
	const std::string &error = {}, const std::string &currentPath = {});

} // namespace sowa::editor
