#pragma once

#include <filesystem>
#include <vector>

#include "core/filesystem/fs_types.hpp"
#include "core/types.hpp"

namespace sowa {

class FileServer {
  public:
	virtual ~FileServer() = default;

	virtual FsResult<Ref<FileData>> Load(const std::filesystem::path &path) = 0;

	virtual FsResult<std::vector<FileEntry>> ReadDirectory(const std::filesystem::path &path) {
		(void)path;
		return {{}, FsError::NotSupported};
	}

	virtual FsError Save(const std::filesystem::path &path, const FileData &data) {
		(void)path;
		(void)data;
		return FsError::NotSupported;
	}

	virtual FsError CreateFile(const std::filesystem::path &path) {
		(void)path;
		return FsError::NotSupported;
	}

	virtual FsError CreateDirectory(const std::filesystem::path &path) {
		(void)path;
		return FsError::NotSupported;
	}

	virtual FsError Move(const std::filesystem::path &from, const std::filesystem::path &to) {
		(void)from;
		(void)to;
		return FsError::NotSupported;
	}

	// Removes files and directory trees. Implementations must reject their
	// mounted root; an empty virtual path must never erase the whole mount.
	virtual FsError RemoveAll(const std::filesystem::path &path) {
		(void)path;
		return FsError::NotSupported;
	}
};

} // namespace sowa
