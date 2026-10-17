#pragma once

#include <filesystem>
#include <string_view>

#include "core/filesystem/fs_types.hpp"
#include "core/filesystem/zip_path.hpp"

namespace sowa {

// Queues entries; Build streams disk files into a new archive. The first error
// is sticky and is returned by Error()/Build(). Existing output is never replaced.
class ZipBuilder {
  public:
	// 0 = stored, 1 = fastest, 9 = smallest. Default is 6.
	ZipBuilder &CompressionLevel(int level);
	ZipBuilder &AddData(std::string_view archivePath, FileData data);
	ZipBuilder &AddFile(std::string_view archivePath, const std::filesystem::path &source);
	// Adds the contents recursively, including empty directories. Symlinks and
	// special files are rejected. An optional prefix places the tree in a subfolder.
	ZipBuilder &AddDirectory(const std::filesystem::path &source, std::string_view prefix = {});

	FsError Error() const {
		return _error;
	}

	FsError Build(const std::filesystem::path &output) const;

  private:
	struct Entry {
		std::string name;
		std::filesystem::path source;
		FileData data;
		bool directory = false;
	};

	bool addPath(std::string_view path, bool directory, std::string &name);

	int _compressionLevel = 6;
	FsError _error = FsError::Ok;
	detail::ZipIndex _paths;
	std::vector<Entry> _entries;
};

} // namespace sowa
