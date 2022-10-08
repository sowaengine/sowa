#include "Core/ExportGenerator.hpp"
#include "Core/ProjectSettings.hpp"
#include "Utils/File.hpp"
#include <iostream>

namespace Ease {
bool ExportGenerator::BeginExport(ExportPlatform platform) {
	_canExport = true;
	_warningCount = 0;

	if (_platform != ExportPlatform::NONE) {
		std::cout << "An export process is already running. Please end it before beginning another process" << std::endl;
		_canExport = false;
		return false;
	}
	_platform = platform;

	_buildDir = Ease::File::Path("res://build/");
	_buildDir /= GetPlatformString();

	if (!std::filesystem::is_directory(_buildDir)) {
		// if it exists, it's probably a file
		if (std::filesystem::exists(_buildDir)) {
			std::cout << "Failed to create build directory, there is a file named " << _buildDir << std::endl;
			_canExport = false;
			return false;
		}

		// if both directory doesn't exists and can't create, either we dont have permission or it is a file.
		if (!std::filesystem::create_directories(_buildDir)) {
			std::cout << "Failed to create build directory, check your permission on " << _buildDir << std::endl;
			_canExport = false;
			return false;
		}
	} else {
		// if there is a directory, clear it
		std::uintmax_t removeCount = std::filesystem::remove_all(_buildDir); // remove_all also counts directory itself
		if (removeCount > 1)
			std::cout << "Build directory has containing files. Cleared build directory. Removed " << removeCount - 1 << " files" << std::endl;

		if (!std::filesystem::create_directories(_buildDir)) {
			std::cout << "Failed to create build directory, check your permission on " << _buildDir << std::endl;
			_canExport = false;
			return false;
		}
	}

	std::cout << "Begin exporting to " << _buildDir << std::endl;

	// add export template to root directory
	std::filesystem::path exportTemplatePath = "abs://build/Sowa Engine";
	// exportTemplatePath /= GetPlatformString();
	if (!AddFileTo(exportTemplatePath, ProjectSettings::get_singleton()._application.Name, false)) {
		std::cout << "Can not include export template: " << exportTemplatePath << std::endl;
		_canExport = false;
		return false;
	}
	return true;
}

bool ExportGenerator::EndExport() {
	if (_platform == ExportPlatform::NONE) {
		std::cout << "There is no export process running with this instance." << std::endl;
		return false;
	}
	if (!_canExport) {
		std::cout << "An error occurred on build process" << std::endl;
		return false;
	}
	if (_warningCount > 0) {
		std::cout << "Build process has " << _warningCount << " warnings." << std::endl;
	}

	std::cout << "Export process successful" << std::endl;
	_platform = ExportPlatform::NONE;
	return true;
}

bool ExportGenerator::AddFile(const std::filesystem::path &filepath, bool useGamePack /*= true*/) {
	Ease::File::FilePathData pathData{};
	Ease::File::Path(filepath, &pathData);

	return AddFileTo(filepath, pathData.relativePath, useGamePack);
}

bool ExportGenerator::AddFileTo(const std::filesystem::path &in_srcPath, const std::filesystem::path &in_dstPath, bool useGamePack /*= true*/) {
	std::filesystem::path srcPath = Ease::File::Path(in_srcPath);
	std::filesystem::path dstPath = _buildDir / in_dstPath;

	if (std::filesystem::is_regular_file(srcPath)) {
		if (useGamePack) {

		} else {
			if (!std::filesystem::is_directory(dstPath.parent_path())) {
				std::filesystem::create_directories(dstPath.parent_path());
			}
			std::filesystem::copy_file(srcPath, dstPath);
		}
	} else {
		std::cout << "Given path " << srcPath << " is not a file" << std::endl;
		_warningCount++;
		return false;
	}
	std::cout << "Including " << srcPath << ", dst: " << dstPath << std::endl;
	return true;
}

std::string ExportGenerator::GetPlatformString() {
	if (_platform == ExportPlatform::LINUX)
		return "linux";
	return "unknown";
}
} // namespace Ease
