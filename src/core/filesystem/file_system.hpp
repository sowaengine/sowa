#pragma once

#include "core/filesystem/file_watcher.hpp"
#include <eventpp/callbacklist.h>
#include <filesystem>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "core/filesystem/file_server.hpp"
#include "core/filesystem/fs_types.hpp"
#include "core/types.hpp"

namespace sowa {

struct FileSystemEvents {
	using Paths = eventpp::CallbackList<void(const std::string &)>;
	Paths read;
	Paths changed;
};

class FileSystem {
  public:
	// Events outlive the filesystem when a subscriber tears down later.
	std::shared_ptr<FileSystemEvents> Events() const {
		return _events;
	}

	void EnableHotReload(bool enabled = true);
	void DispatchChanges();
	std::string Normalize(std::string_view path) const;
	void Register(std::string scheme, Scope<FileServer> server);
	void MountFolder(std::string scheme, std::filesystem::path root);
	// Leaves any existing mount intact if opening or indexing the ZIP fails.
	FsError MountZip(std::string scheme, const std::filesystem::path &archivePath);

	FileServer *Get(std::string_view scheme) const;
	bool Has(std::string_view scheme) const;

	FsResult<PathData> Resolve(std::string_view path) const;

	FsResult<Ref<FileData>> Load(std::string_view path);
	FsResult<std::vector<FileEntry>> ReadDirectory(std::string_view path);
	FsError Save(std::string_view path, const FileData &data);
	FsError CreateFile(std::string_view path);
	FsError CreateDirectory(std::string_view path);
	FsError Move(std::string_view from, std::string_view to);
	FsError RemoveAll(std::string_view path);

  private:
	FsResult<FileServer *> serverFor(const PathData &path) const;

	std::shared_ptr<FileSystemEvents> _events = std::make_shared<FileSystemEvents>();
	std::unique_ptr<FileWatcher> _watcher;
	std::unordered_map<std::string, Scope<FileServer>> _servers;
};

} // namespace sowa
