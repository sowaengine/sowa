#include "core/filesystem/zip_builder.hpp"

#include <cerrno>
#include <cstdio>
#include <miniz.h>

#include "core/filesystem/zip_path.hpp"
#include "core/scope_exit.hpp"

namespace sowa {
namespace {

FsError checkSource(const std::filesystem::path &path, bool directory) {
	std::error_code ec;
	const auto status = std::filesystem::symlink_status(path, ec);
	if (ec == std::errc::no_such_file_or_directory) {
		return FsError::NotFound;
	}
	if (ec) {
		return FsError::IoError;
	}
	if (directory && !std::filesystem::is_directory(status)) {
		return FsError::NotADirectory;
	}
	if (!directory && !std::filesystem::is_regular_file(status)) {
		return FsError::NotAFile;
	}
	return FsError::Ok;
}

} // namespace

ZipBuilder &ZipBuilder::CompressionLevel(int level) {
	if (_error != FsError::Ok) {
		return *this;
	}

	if (level < 0 || level > 9) {
		_error = FsError::NotSupported;
	} else {
		_compressionLevel = level;
	}

	return *this;
}

bool ZipBuilder::addPath(std::string_view path, bool directory, std::string &name) {
	if (_error != FsError::Ok) {
		return false;
	}

	auto normalized = detail::ZipPath(path);
	if (!normalized || normalized.value.empty() || (!directory && path.ends_with('/')) ||
		normalized.value.size() > 65534) {
		_error = FsError::InvalidPath;
		return false;
	}
	name = std::move(normalized.value);
	_error = detail::AddZipPath(_paths, name, {.isDirectory = directory});
	return _error == FsError::Ok;
}

ZipBuilder &ZipBuilder::AddData(std::string_view archivePath, FileData data) {
	std::string name;
	if (addPath(archivePath, false, name)) {
		_entries.push_back({.name = std::move(name), .data = std::move(data)});
	}

	return *this;
}

ZipBuilder &ZipBuilder::AddFile(std::string_view archivePath, const std::filesystem::path &source) {
	if (_error != FsError::Ok) {
		return *this;
	}

	_error = checkSource(source, false);
	std::string name;
	if (!addPath(archivePath, false, name)) {
		return *this;
	}

	std::error_code ec;
	const auto absolute = std::filesystem::absolute(source, ec);
	if (ec) {
		_error = FsError::IoError;
		return *this;
	}

	_entries.push_back({.name = std::move(name), .source = absolute});

	return *this;
}

ZipBuilder &ZipBuilder::AddDirectory(const std::filesystem::path &source, std::string_view prefix) {
	if (_error != FsError::Ok) {
		return *this;
	}

	_error = checkSource(source, true);
	if (_error != FsError::Ok) {
		return *this;
	}

	auto normalized = detail::ZipPath(prefix);
	if (!normalized) {
		_error = normalized.error;
		return *this;
	}

	const auto base = normalized.value;
	if (!base.empty()) {
		std::string name;
		if (!addPath(base, true, name)) {
			return *this;
		}
		_entries.push_back({.name = std::move(name), .directory = true});
	}

	std::error_code ec;
	std::filesystem::recursive_directory_iterator it(source, ec);
	const std::filesystem::recursive_directory_iterator end;
	for (; !ec && it != end; it.increment(ec)) {
		const auto &child = it->path();
		const auto relative = detail::ZipPathString(child.lexically_relative(source));
		const auto name = base.empty() ? relative : base + '/' + relative;
		const auto childStatus = std::filesystem::symlink_status(child, ec);
		if (ec) {
			_error = FsError::IoError;
			break;
		}
		if (std::filesystem::is_directory(childStatus)) {
			std::string normalizedName;
			if (!addPath(name, true, normalizedName)) {
				break;
			}
			_entries.push_back({.name = std::move(normalizedName), .directory = true});
		} else {
			AddFile(name, child);
		}
		if (_error != FsError::Ok) {
			break;
		}
	}

	if (ec) {
		_error = FsError::IoError;
	}

	return *this;
}

FsError ZipBuilder::Build(const std::filesystem::path &output) const {
	if (_error != FsError::Ok) {
		return _error;
	}

	if (output.empty()) {
		return FsError::InvalidPath;
	}

	std::error_code ec;
	mz_zip_archive archive{};
	// Exclusive creation closes the exists/open race and never truncates another export.
#ifdef _WIN32
	std::FILE *file = _wfopen(output.c_str(), L"wbx");
#else
	std::FILE *file = std::fopen(output.c_str(), "wbx");
#endif
	if (!file) {
		return errno == EEXIST ? FsError::AlreadyExists : FsError::IoError;
	}
	bool completed = false;
	auto cleanup = MakeScopeExit([&] {
		if (archive.m_pState) {
			mz_zip_writer_end(&archive);
		}
		if (file) {
			std::fclose(file);
		}
		if (!completed) {
			std::filesystem::remove(output, ec);
		}
	});
	if (!mz_zip_writer_init_cfile(&archive, file, 0)) {
		return FsError::IoError;
	}
	for (const auto &entry : _entries) {
		bool written = false;
		if (entry.directory) {
			written = mz_zip_writer_add_mem(&archive, (entry.name + '/').c_str(), nullptr, 0, 0);
		} else if (!entry.source.empty()) {
			const auto source = detail::ZipPathString(entry.source);
			written = mz_zip_writer_add_file(&archive, entry.name.c_str(), source.c_str(), nullptr,
				0, _compressionLevel);
		} else {
			written = mz_zip_writer_add_mem(&archive, entry.name.c_str(), entry.data.Data(),
				entry.data.Size(), _compressionLevel);
		}
		if (!written) {
			return FsError::IoError;
		}
	}

	if (!mz_zip_writer_finalize_archive(&archive) || !mz_zip_writer_end(&archive)) {
		return FsError::IoError;
	}

	const auto closeResult = std::fclose(file);
	file = nullptr;
	if (closeResult != 0) {
		return FsError::IoError;
	}

	completed = true;
	return FsError::Ok;
}

} // namespace sowa
