#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace sowa {

enum class FsError {
	Ok,
	InvalidPath,
	NotFound,
	AlreadyExists,
	NotAFile,
	NotADirectory,
	NotSupported,
	IoError,
};

inline const char *FsErrorToString(FsError error) {
	switch (error) {
	case FsError::Ok:
		return "ok";
	case FsError::InvalidPath:
		return "invalid path";
	case FsError::NotFound:
		return "not found";
	case FsError::AlreadyExists:
		return "already exists";
	case FsError::NotAFile:
		return "not a file";
	case FsError::NotADirectory:
		return "not a directory";
	case FsError::NotSupported:
		return "not supported";
	case FsError::IoError:
		return "I/O error";
	}
	return "unknown error";
}

template <typename T> struct FsResult {
	T value{};
	FsError error = FsError::Ok;

	explicit operator bool() const {
		return error == FsError::Ok;
	}
};

struct FileData {
	std::vector<std::byte> bytes;

	std::byte *Data() {
		return bytes.data();
	}

	const std::byte *Data() const {
		return bytes.data();
	}

	std::size_t Size() const {
		return bytes.size();
	}
};

struct FileEntry {
	std::string name;
	bool isDirectory = false;
};

struct PathData {
	std::string scheme;
	std::string path;
};

} // namespace sowa
