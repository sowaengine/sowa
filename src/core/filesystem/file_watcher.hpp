#pragma once

#include <filesystem>
#include <functional>
#include <memory>
#include <string>

struct uv_fs_event_s;

namespace sowa {

// Main-thread native directory subscriptions. Watches parent directories so
// atomic file replacements keep working. Dispatch pumps queued OS events;
// it never scans directories or compares file timestamps.
class FileWatcher {
  public:
	FileWatcher();
	// Native start operation is injectable so backend failures can be tested
	// without exhausting host descriptors. Production uses uv_fs_event_start.
	using NativeCallback = void (*)(uv_fs_event_s *, const char *, int, int);
	using NativeStart = std::function<int(uv_fs_event_s *, NativeCallback, const char *, unsigned)>;
	explicit FileWatcher(NativeStart start);
	~FileWatcher();
	FileWatcher(const FileWatcher &) = delete;
	FileWatcher &operator=(const FileWatcher &) = delete;
	// True means subscribed, including when native startup is waiting for a retry.
	bool Watch(const std::filesystem::path &file, std::string key);
	void Dispatch(const std::function<void(const std::string &)> &changed);

  private:
	struct Impl;
	std::unique_ptr<Impl> _impl;
};

} // namespace sowa
