#include "core/filesystem/file_watcher.hpp"

#include <algorithm>
#include <chrono>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "core/log.hpp"
#ifndef __EMSCRIPTEN__
#include <uv.h>
#endif

namespace sowa {

struct FileWatcher::Impl {
#ifndef __EMSCRIPTEN__
	using Clock = std::chrono::steady_clock;

	struct Directory {
		uv_fs_event_t handle{};
		Impl *owner = nullptr;
		std::string path;
		bool watching = false;
		int lastError = 0;
		std::unordered_map<std::string, std::unordered_set<std::string>> files;
	};

	uv_loop_t loop{};
	uv_timer_t retryTimer{};
	bool ready = false;
	uint64_t retryDelay = 100;
	NativeStart start;

	explicit Impl(NativeStart nativeStart) : start(std::move(nativeStart)) {
		if (uv_loop_init(&loop) != 0) {
			return;
		}
		if (uv_timer_init(&loop, &retryTimer) != 0) {
			uv_loop_close(&loop);
			return;
		}
		retryTimer.data = this;
		ready = true;
	}

	std::unordered_map<std::string, Directory *> directories;
	std::unordered_map<std::string, Clock::time_point> pending;

	static void onChange(uv_fs_event_t *handle, const char *filename, int, int status) {
		auto &dir = *static_cast<Impl::Directory *>(handle->data);
		if (status < 0) {
			uv_fs_event_stop(&dir.handle);
			dir.watching = false;
			dir.owner->retry(dir, status);
			return;
		}
		if (dir.lastError != 0) {
			dir.owner->retryDelay = 100;
			dir.lastError = 0;
		}
		// Some native backends cannot identify the changed entry.
		// Invalidate subscribed files only in that directory.
		for (const auto &[name, keys] : dir.files) {
			if (filename && name != filename) {
				continue;
			}
			for (const auto &key : keys) {
				dir.owner->pending[key] = Impl::Clock::now();
			}
		}
	}

	void retry(Directory &directory, int error) {
		if (directory.lastError != error) {
			Log::Warn("Cannot watch '{}': {}; retrying native subscription", directory.path,
				uv_strerror(error));
			directory.lastError = error;
		}
		if (!uv_is_active(reinterpret_cast<uv_handle_t *>(&retryTimer))) {
			uv_timer_start(&retryTimer, onRetry, retryDelay, 0);
			retryDelay = std::min<uint64_t>(retryDelay * 2, 5000);
		}
	}

	bool startDirectory(Directory &directory) {
		const int status = start(&directory.handle, onChange, directory.path.c_str(), 0);
		if (status != 0) {
			retry(directory, status);
			return false;
		}
		directory.watching = true;
		return true;
	}

	static void onRetry(uv_timer_t *timer) {
		auto &owner = *static_cast<Impl *>(timer->data);
		for (const auto &[path, directory] : owner.directories) {
			if (directory->watching || !owner.startDirectory(*directory)) {
				continue;
			}
			// Events during the outage may be lost. Invalidate each subscription
			// once after reattachment; normal operation remains event-driven.
			for (const auto &[name, keys] : directory->files) {
				for (const auto &key : keys) {
					owner.pending[key] = Clock::now();
				}
			}
		}
	}

	static void onClose(uv_handle_t *handle) {
		delete static_cast<Directory *>(handle->data);
	}

	~Impl() {
		if (!ready) {
			return;
		}
		uv_timer_stop(&retryTimer);
		uv_close(reinterpret_cast<uv_handle_t *>(&retryTimer), nullptr);
		for (const auto &[path, directory] : directories) {
			uv_fs_event_stop(&directory->handle);
			uv_close(reinterpret_cast<uv_handle_t *>(&directory->handle), onClose);
		}
		uv_run(&loop, UV_RUN_DEFAULT);
		uv_loop_close(&loop);
	}
#else
	explicit Impl(NativeStart) {
	}
#endif
};

FileWatcher::FileWatcher()
#ifndef __EMSCRIPTEN__
	: FileWatcher(uv_fs_event_start)
#else
	: FileWatcher(NativeStart{})
#endif
{
}

FileWatcher::FileWatcher(NativeStart start) : _impl(std::make_unique<Impl>(std::move(start))) {
}

FileWatcher::~FileWatcher() = default;

bool FileWatcher::Watch(const std::filesystem::path &file, std::string key) {
#ifdef __EMSCRIPTEN__
	return false;
#else
	if (!_impl->ready) {
		return false;
	}
	std::error_code error;
	const auto absolute = std::filesystem::absolute(file, error).lexically_normal();
	if (error) {
		return false;
	}
	const std::string parent = absolute.parent_path().string();
	auto found = _impl->directories.find(parent);
	Impl::Directory *directory = nullptr;
	if (found == _impl->directories.end()) {
		directory = new Impl::Directory;
		directory->owner = _impl.get();
		if (uv_fs_event_init(&_impl->loop, &directory->handle) != 0) {
			delete directory;
			return false;
		}
		directory->handle.data = directory;
		directory->path = parent;
		_impl->directories.emplace(parent, directory);
		_impl->startDirectory(*directory);
	} else {
		directory = found->second;
	}
	directory->files[absolute.filename().string()].insert(std::move(key));
	return true;
#endif
}

void FileWatcher::Dispatch(const std::function<void(const std::string &)> &changed) {
#ifndef __EMSCRIPTEN__
	if (!_impl->ready) {
		return;
	}
	uv_run(&_impl->loop, UV_RUN_NOWAIT);
	std::vector<std::string> ready;
	const auto now = Impl::Clock::now();
	for (auto it = _impl->pending.begin(); it != _impl->pending.end();) {
		// Collapse editor write/rename bursts and avoid reading mid-save.
		if (now - it->second < std::chrono::milliseconds(100)) {
			++it;
			continue;
		}
		ready.push_back(it->first);
		it = _impl->pending.erase(it);
	}
	for (const auto &key : ready) {
		changed(key);
	}
#endif
}

} // namespace sowa
