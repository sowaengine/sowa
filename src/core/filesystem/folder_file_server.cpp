#include "core/filesystem/folder_file_server.hpp"

#include <algorithm>
#include <fstream>

namespace sowa {

FolderFileServer::FolderFileServer(std::filesystem::path root) : _root(std::move(root)) {
}

std::filesystem::path FolderFileServer::resolve(const std::filesystem::path &path) const {
	return _root / path;
}

FsResult<Ref<FileData>> FolderFileServer::Load(const std::filesystem::path &path) {
	const std::filesystem::path full = resolve(path);

	std::error_code ec;
	if (!std::filesystem::exists(full, ec)) {
		return {nullptr, FsError::NotFound};
	}
	if (std::filesystem::is_directory(full, ec)) {
		return {nullptr, FsError::NotAFile};
	}

	std::ifstream file(full, std::ios::binary);
	if (!file) {
		return {nullptr, FsError::IoError};
	}

	file.seekg(0, std::ios::end);
	const auto size = file.tellg();
	file.seekg(0, std::ios::beg);
	if (size < 0) {
		return {nullptr, FsError::IoError};
	}

	auto data = MakeRef<FileData>();
	data->bytes.resize(static_cast<std::size_t>(size));
	if (size > 0) {
		file.read(reinterpret_cast<char *>(data->bytes.data()), size);
		if (!file) {
			return {nullptr, FsError::IoError};
		}
	}

	return {std::move(data), FsError::Ok};
}

FsResult<std::vector<FileEntry>> FolderFileServer::ReadDirectory(
	const std::filesystem::path &path) {
	const std::filesystem::path full = resolve(path);

	std::error_code ec;
	if (!std::filesystem::exists(full, ec)) {
		return {{}, FsError::NotFound};
	}
	if (!std::filesystem::is_directory(full, ec)) {
		return {{}, FsError::NotADirectory};
	}

	std::vector<FileEntry> entries;
	for (const auto &dirEntry : std::filesystem::directory_iterator(full, ec)) {
		if (ec) {
			return {{}, FsError::IoError};
		}
		entries.push_back(FileEntry{
			.name = dirEntry.path().filename().string(),
			.isDirectory = dirEntry.is_directory(ec),
		});
	}
	if (ec) {
		return {{}, FsError::IoError};
	}

	std::sort(entries.begin(), entries.end(),
		[](const FileEntry &a, const FileEntry &b) { return a.name < b.name; });
	return {std::move(entries), FsError::Ok};
}

FsError FolderFileServer::Save(const std::filesystem::path &path, const FileData &data) {
	const std::filesystem::path full = resolve(path);

	std::error_code ec;
	if (std::filesystem::is_directory(full, ec)) {
		return FsError::NotAFile;
	}

	std::ofstream file(full, std::ios::binary | std::ios::trunc);
	if (!file) {
		return FsError::IoError;
	}

	if (!data.bytes.empty()) {
		file.write(reinterpret_cast<const char *>(data.bytes.data()),
			static_cast<std::streamsize>(data.bytes.size()));
		if (!file) {
			return FsError::IoError;
		}
	}

	return FsError::Ok;
}

FsError FolderFileServer::CreateFile(const std::filesystem::path &path) {
	if (path.empty()) {
		return FsError::InvalidPath;
	}
	const std::filesystem::path full = resolve(path);
	std::error_code ec;
	if (std::filesystem::exists(full, ec)) {
		return FsError::AlreadyExists;
	}
	if (ec) {
		return FsError::IoError;
	}
	if (!std::filesystem::is_directory(full.parent_path(), ec)) {
		return ec ? FsError::IoError : FsError::NotFound;
	}

	std::ofstream file(full, std::ios::binary);
	return file ? FsError::Ok : FsError::IoError;
}

FsError FolderFileServer::CreateDirectory(const std::filesystem::path &path) {
	if (path.empty()) {
		return FsError::InvalidPath;
	}
	const std::filesystem::path full = resolve(path);
	std::error_code ec;
	if (std::filesystem::exists(full, ec)) {
		return FsError::AlreadyExists;
	}
	if (ec) {
		return FsError::IoError;
	}
	if (!std::filesystem::create_directory(full, ec)) {
		return ec ? FsError::IoError : FsError::NotFound;
	}
	return FsError::Ok;
}

FsError FolderFileServer::Move(const std::filesystem::path &from, const std::filesystem::path &to) {
	if (from.empty() || to.empty()) {
		return FsError::InvalidPath;
	}
	const std::filesystem::path fullFrom = resolve(from);
	const std::filesystem::path fullTo = resolve(to);
	std::error_code ec;
	if (!std::filesystem::exists(fullFrom, ec)) {
		return ec ? FsError::IoError : FsError::NotFound;
	}
	if (std::filesystem::exists(fullTo, ec)) {
		return FsError::AlreadyExists;
	}
	if (ec) {
		return FsError::IoError;
	}
	std::filesystem::rename(fullFrom, fullTo, ec);
	return ec ? FsError::IoError : FsError::Ok;
}

FsError FolderFileServer::RemoveAll(const std::filesystem::path &path) {
	if (path.empty()) {
		return FsError::InvalidPath;
	}
	const std::filesystem::path full = resolve(path);
	std::error_code ec;
	if (!std::filesystem::exists(full, ec)) {
		return ec ? FsError::IoError : FsError::NotFound;
	}
	const std::uintmax_t removed = std::filesystem::remove_all(full, ec);
	if (ec) {
		return FsError::IoError;
	}
	return removed > 0 ? FsError::Ok : FsError::NotFound;
}

} // namespace sowa
