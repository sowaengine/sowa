#include "Utils/Dialog.hpp"
#include "tinyfiledialogs.h"

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace Ease {
namespace Dialog {

std::string OpenFileDialog(
	const std::string &title,
	const std::string &_defaultPath,
	int filterCount,
	const std::vector<std::string> &filterPatterns,
	bool multipleSelection,
	bool *isInDirectory /*= nullptr*/) {

	std::filesystem::path defaultPath = _defaultPath;
	if (std::filesystem::exists(_defaultPath)) {
		defaultPath = std::filesystem::absolute(_defaultPath);
	}

	char *path = tinyfd_openFileDialog(title.c_str(), defaultPath.string().c_str(), filterCount, (const char *const *)filterPatterns.data(), NULL, multipleSelection ? 1 : 0);
	if (path == nullptr)
		return "";

	std::string relativePath = std::filesystem::relative(path, defaultPath).string();
	if (isInDirectory != nullptr) {
		if (relativePath.substr(0, 3) == "../") {
			*isInDirectory = false;
		} else {
			*isInDirectory = true;
		}
	}
	return relativePath;
}
} // namespace Dialog
} // namespace Ease
