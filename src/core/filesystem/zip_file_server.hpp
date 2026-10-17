#pragma once

#include <miniz.h>

#include "core/filesystem/file_server.hpp"
#include "core/filesystem/zip_path.hpp"

namespace sowa {

// Keeps the archive open and extracts only the requested file. Mutations inherit
// FileServer's NotSupported defaults. Calls on one server must be serialized.
class ZipFileServer : public FileServer {
  public:
	explicit ZipFileServer(const std::filesystem::path &archivePath);
	~ZipFileServer() override;
	ZipFileServer(const ZipFileServer &) = delete;
	ZipFileServer &operator=(const ZipFileServer &) = delete;

	FsError Error() const;
	FsResult<Ref<FileData>> Load(const std::filesystem::path &path) override;
	FsResult<std::vector<FileEntry>> ReadDirectory(const std::filesystem::path &path) override;

  private:
	FsError open(const std::filesystem::path &archivePath);

	mz_zip_archive _archive{};
	detail::ZipIndex _entries;
	FsError _error = FsError::Ok;
};

} // namespace sowa
