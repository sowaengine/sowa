#include "core/filesystem/file_system.hpp"

#include "core/filesystem/folder_file_server.hpp"
#include "core/filesystem/zip_file_server.hpp"
#include "core/string.hpp"

namespace sowa {

void FileSystem::EnableHotReload(bool enabled) {
	if (enabled && !_watcher) {
		_watcher = std::make_unique<FileWatcher>();
	}
	if (!enabled) {
		_watcher.reset();
	}
}

void FileSystem::DispatchChanges() {
	if (_watcher) {
		_watcher->Dispatch([this](const std::string &path) { _events->changed(path); });
	}
}

std::string FileSystem::Normalize(std::string_view path) const {
	const auto resolved = Resolve(path);
	if (!resolved) {
		return {};
	}
	return resolved.value.scheme + "://" +
		   std::filesystem::path(resolved.value.path).lexically_normal().generic_string();
}

void FileSystem::Register(std::string scheme, Scope<FileServer> server) {
	_servers[std::move(scheme)] = std::move(server);
}

void FileSystem::MountFolder(std::string scheme, std::filesystem::path root) {
	Register(std::move(scheme), MakeScope<FolderFileServer>(std::move(root)));
}

FsError FileSystem::MountZip(std::string scheme, const std::filesystem::path &archivePath) {
	auto server = MakeScope<ZipFileServer>(archivePath);
	if (server->Error() != FsError::Ok) {
		return server->Error();
	}
	Register(std::move(scheme), std::move(server));
	return FsError::Ok;
}

FileServer *FileSystem::Get(std::string_view scheme) const {
	const auto it = _servers.find(std::string(scheme));
	if (it == _servers.end()) {
		return nullptr;
	}
	return it->second.get();
}

bool FileSystem::Has(std::string_view scheme) const {
	return _servers.find(std::string(scheme)) != _servers.end();
}

FsResult<PathData> FileSystem::Resolve(std::string_view path) const {
	PathData data;
	const auto parts = String::Split(path, "://");

	if (parts.size() != 2) {
		return {{}, FsError::InvalidPath};
	}
	if (parts[0].empty()) {
		return {{}, FsError::InvalidPath};
	}

	data.scheme = parts[0];
	data.path = parts[1];

	// Strip leading slashes.
	const auto first = data.path.find_first_not_of('/');
	if (first == std::string::npos) {
		data.path.clear();
	} else if (first > 0) {
		data.path = data.path.substr(first);
	}

	// Reject ".." path escapes.
	const std::filesystem::path relative(data.path);
	for (const auto &part : relative) {
		if (part == "..") {
			return {{}, FsError::InvalidPath};
		}
	}

	data.path = relative.generic_string();
	if (data.path == ".") {
		data.path.clear();
	}

	return {std::move(data), FsError::Ok};
}

FsResult<FileServer *> FileSystem::serverFor(const PathData &path) const {
	FileServer *server = Get(path.scheme);
	if (!server) {
		return {nullptr, FsError::InvalidPath};
	}
	return {server, FsError::Ok};
}

FsResult<Ref<FileData>> FileSystem::Load(std::string_view path) {
	auto resolved = Resolve(path);
	if (!resolved) {
		return {nullptr, resolved.error};
	}

	auto server = serverFor(resolved.value);
	if (!server) {
		return {nullptr, server.error};
	}

	const std::string key = Normalize(path);
	_events->read(key);
	if (_watcher) {
		if (auto *folder = dynamic_cast<FolderFileServer *>(server.value)) {
			_watcher->Watch(folder->Root() / resolved.value.path, key);
		}
	}
	return server.value->Load(resolved.value.path);
}

FsResult<std::vector<FileEntry>> FileSystem::ReadDirectory(std::string_view path) {
	auto resolved = Resolve(path);
	if (!resolved) {
		return {{}, resolved.error};
	}

	auto server = serverFor(resolved.value);
	if (!server) {
		return {{}, server.error};
	}

	return server.value->ReadDirectory(resolved.value.path);
}

FsError FileSystem::Save(std::string_view path, const FileData &data) {
	auto resolved = Resolve(path);
	if (!resolved) {
		return resolved.error;
	}

	auto server = serverFor(resolved.value);
	if (!server) {
		return server.error;
	}

	// New editor documents may be saved before ever being read.
	if (_watcher) {
		if (auto *folder = dynamic_cast<FolderFileServer *>(server.value)) {
			_watcher->Watch(folder->Root() / resolved.value.path, Normalize(path));
		}
	}
	return server.value->Save(resolved.value.path, data);
}

FsError FileSystem::CreateFile(std::string_view path) {
	auto resolved = Resolve(path);
	if (!resolved) {
		return resolved.error;
	}
	auto server = serverFor(resolved.value);
	if (!server) {
		return server.error;
	}
	return server.value->CreateFile(resolved.value.path);
}

FsError FileSystem::CreateDirectory(std::string_view path) {
	auto resolved = Resolve(path);
	if (!resolved) {
		return resolved.error;
	}
	auto server = serverFor(resolved.value);
	if (!server) {
		return server.error;
	}
	return server.value->CreateDirectory(resolved.value.path);
}

FsError FileSystem::Move(std::string_view from, std::string_view to) {
	auto resolvedFrom = Resolve(from);
	auto resolvedTo = Resolve(to);
	if (!resolvedFrom) {
		return resolvedFrom.error;
	}
	if (!resolvedTo) {
		return resolvedTo.error;
	}
	if (resolvedFrom.value.scheme != resolvedTo.value.scheme) {
		return FsError::NotSupported;
	}
	auto server = serverFor(resolvedFrom.value);
	if (!server) {
		return server.error;
	}
	return server.value->Move(resolvedFrom.value.path, resolvedTo.value.path);
}

FsError FileSystem::RemoveAll(std::string_view path) {
	auto resolved = Resolve(path);
	if (!resolved) {
		return resolved.error;
	}
	auto server = serverFor(resolved.value);
	if (!server) {
		return server.error;
	}
	return server.value->RemoveAll(resolved.value.path);
}

} // namespace sowa
