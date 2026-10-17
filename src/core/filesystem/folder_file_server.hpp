#pragma once

#include <filesystem>

#include "core/filesystem/file_server.hpp"

namespace sowa {

class FolderFileServer : public FileServer {
  public:
	explicit FolderFileServer(std::filesystem::path root);

	const std::filesystem::path &Root() const {
		return _root;
	}

	FsResult<Ref<FileData>> Load(const std::filesystem::path &path) override;
	FsResult<std::vector<FileEntry>> ReadDirectory(const std::filesystem::path &path) override;
	FsError Save(const std::filesystem::path &path, const FileData &data) override;
	FsError CreateFile(const std::filesystem::path &path) override;
	FsError CreateDirectory(const std::filesystem::path &path) override;
	FsError Move(const std::filesystem::path &from, const std::filesystem::path &to) override;
	FsError RemoveAll(const std::filesystem::path &path) override;

  private:
	std::filesystem::path resolve(const std::filesystem::path &path) const;

	std::filesystem::path _root;
};

} // namespace sowa
