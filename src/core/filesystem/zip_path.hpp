#pragma once

#include <filesystem>
#include <map>
#include <string_view>

#include "core/filesystem/fs_types.hpp"

namespace sowa::detail {

inline std::string ZipPathString(const std::filesystem::path &path) {
	const auto utf8 = path.generic_u8string();
	return {reinterpret_cast<const char *>(utf8.data()), utf8.size()};
}

struct ZipEntry {
	bool isDirectory = false;
	unsigned fileIndex = 0;
};

using ZipIndex = std::map<std::string, ZipEntry>;

// ZIP names always use forward slashes and are case sensitive on every platform.
inline FsResult<std::string> ZipPath(std::string_view path) {
	if ((!path.empty() && path.front() == '/') ||
		path.find_first_of("\\:") != std::string_view::npos ||
		path.find('\0') != std::string_view::npos) {
		return {{}, FsError::InvalidPath};
	}

	std::string result;
	while (!path.empty()) {
		const auto end = path.find('/');
		const auto part = path.substr(0, end);
		if (part == "..") {
			return {{}, FsError::InvalidPath};
		}
		if (!part.empty() && part != ".") {
			if (!result.empty()) {
				result += '/';
			}
			result += part;
		}
		if (end == std::string_view::npos) {
			break;
		}
		path.remove_prefix(end + 1);
	}

	return {std::move(result), FsError::Ok};
}

// Includes implicit parent directories; rejects file/directory collisions.
inline FsError AddZipPath(ZipIndex &paths, const std::string &name, ZipEntry entry) {
	const auto found = paths.find(name);
	if (found != paths.end() && !(entry.isDirectory && found->second.isDirectory)) {
		return FsError::AlreadyExists;
	}
	for (auto end = name.find('/'); end != std::string::npos; end = name.find('/', end + 1)) {
		const auto parent = name.substr(0, end);
		if (const auto it = paths.find(parent); it != paths.end() && !it->second.isDirectory) {
			return FsError::AlreadyExists;
		}
		paths[parent].isDirectory = true;
	}
	paths[name] = entry;
	return FsError::Ok;
}

} // namespace sowa::detail
