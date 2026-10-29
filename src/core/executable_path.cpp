#include "core/executable_path.hpp"

#include <string>
#include <system_error>

#if defined(_WIN32)
#include <windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#endif

namespace sowa {

std::filesystem::path ExecutablePath() {
	std::filesystem::path path;
#if defined(_WIN32)
	std::wstring buffer(MAX_PATH, L'\0');
	for (;;) {
		const DWORD length =
			GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
		if (length == 0) {
			return {};
		}
		if (length < buffer.size()) {
			buffer.resize(length);
			break;
		}
		buffer.resize(buffer.size() * 2);
	}
	path = buffer;
#elif defined(__APPLE__)
	uint32_t size = 0;
	_NSGetExecutablePath(nullptr, &size);
	std::string buffer(size, '\0');
	if (_NSGetExecutablePath(buffer.data(), &size) != 0) {
		return {};
	}
	path = buffer.c_str();
#elif defined(__linux__)
	std::error_code linkError;
	path = std::filesystem::read_symlink("/proc/self/exe", linkError);
	if (linkError) {
		return {};
	}
#else
	return {};
#endif
	std::error_code ec;
	auto canonical = std::filesystem::canonical(path, ec);
	return ec ? path : canonical;
}

} // namespace sowa
