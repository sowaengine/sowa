#include "core/filesystem/zip_file_server.hpp"

#include <miniz.h>

#include "core/filesystem/zip_path.hpp"

namespace sowa {

ZipFileServer::ZipFileServer(const std::filesystem::path &archivePath) {
	_error = open(archivePath);
}

ZipFileServer::~ZipFileServer() {
	if (_archive.m_pState) {
		mz_zip_reader_end(&_archive);
	}
}

FsError ZipFileServer::open(const std::filesystem::path &archivePath) {
	_entries[""].isDirectory = true;

	std::error_code ec;
	if (!std::filesystem::exists(archivePath, ec)) {
		return ec ? FsError::IoError : FsError::NotFound;
	}

	if (!std::filesystem::is_regular_file(archivePath, ec)) {
		return ec ? FsError::IoError : FsError::NotAFile;
	}

	const auto filename = detail::ZipPathString(archivePath);
	if (!mz_zip_reader_init_file(&_archive, filename.c_str(), 0)) {
		return FsError::IoError;
	}
	for (mz_uint i = 0; i < mz_zip_reader_get_num_files(&_archive); ++i) {
		mz_zip_archive_file_stat stat{};
		if (!mz_zip_reader_file_stat(&_archive, i, &stat)) {
			return FsError::IoError;
		}
		// stat.m_filename is truncated at 512 bytes; query the complete name.
		const auto size = mz_zip_reader_get_filename(&_archive, i, nullptr, 0);
		if (size == 0) {
			return FsError::InvalidPath;
		}
		std::string raw(size, '\0');
		mz_zip_reader_get_filename(&_archive, i, raw.data(), size);
		raw.pop_back();
		auto name = detail::ZipPath(raw);
		if (!name || (name.value.empty() && !stat.m_is_directory)) {
			return FsError::InvalidPath;
		}
		const detail::ZipEntry entry{bool(stat.m_is_directory), i};
		const auto error = detail::AddZipPath(_entries, name.value, entry);
		if (error != FsError::Ok) {
			return error;
		}
	}

	return FsError::Ok;
}

FsError ZipFileServer::Error() const {
	return _error;
}

FsResult<Ref<FileData>> ZipFileServer::Load(const std::filesystem::path &path) {
	if (Error() != FsError::Ok) {
		return {nullptr, Error()};
	}

	auto name = detail::ZipPath(detail::ZipPathString(path));
	if (!name) {
		return {nullptr, name.error};
	}

	const auto found = _entries.find(name.value);
	if (found == _entries.end()) {
		return {nullptr, FsError::NotFound};
	}

	if (found->second.isDirectory) {
		return {nullptr, FsError::NotAFile};
	}

	const auto index = found->second.fileIndex;
	mz_zip_archive_file_stat stat{};
	if (!mz_zip_reader_file_stat(&_archive, index, &stat)) {
		return {nullptr, FsError::IoError};
	}

	if (!stat.m_is_supported || stat.m_is_encrypted) {
		return {nullptr, FsError::NotSupported};
	}

	auto data = MakeRef<FileData>();
	if (stat.m_uncomp_size > data->bytes.max_size()) {
		return {nullptr, FsError::IoError};
	}
	try {
		data->bytes.resize(static_cast<std::size_t>(stat.m_uncomp_size));
	} catch (const std::bad_alloc &) {
		return {nullptr, FsError::IoError};
	}
	// Give miniz a non-null destination even for empty files, and still verify CRC.
	std::byte empty{};
	if (!mz_zip_reader_extract_to_mem(&_archive, index, data->Size() ? data->Data() : &empty,
			data->Size(), 0)) {
		return {nullptr, FsError::IoError};
	}

	return {std::move(data), FsError::Ok};
}

FsResult<std::vector<FileEntry>> ZipFileServer::ReadDirectory(const std::filesystem::path &path) {
	if (Error() != FsError::Ok) {
		return {{}, Error()};
	}

	auto name = detail::ZipPath(detail::ZipPathString(path));
	if (!name) {
		return {{}, name.error};
	}

	const auto found = _entries.find(name.value);
	if (found == _entries.end()) {
		return {{}, FsError::NotFound};
	}

	if (!found->second.isDirectory) {
		return {{}, FsError::NotADirectory};
	}

	const auto prefix = name.value.empty() ? "" : name.value + '/';
	std::vector<FileEntry> entries;
	for (auto it = _entries.lower_bound(prefix);
		it != _entries.end() && it->first.starts_with(prefix); ++it) {
		const auto child = it->first.substr(prefix.size());
		if (!child.empty() && child.find('/') == std::string::npos) {
			entries.push_back({child, it->second.isDirectory});
		}
	}

	return {std::move(entries), FsError::Ok};
}

} // namespace sowa
