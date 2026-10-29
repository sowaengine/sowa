#include <chrono>
#include <fstream>
#include <map>
#include <set>
#include <thread>
#include <vector>

#include <doctest/doctest.h>

#include "core/filesystem/file_watcher.hpp"

#ifndef __EMSCRIPTEN__
#include <uv.h>

using namespace sowa;

namespace {
struct NativeBackend {
	struct Watch {
		uv_fs_event_t *handle;
		FileWatcher::NativeCallback callback;
	};

	int error = 0;
	std::vector<uint64_t> attempts;
	std::map<std::string, Watch> watches;

	int Start(uv_fs_event_t *handle, FileWatcher::NativeCallback callback, const char *path,
		unsigned) {
		attempts.push_back(uv_now(handle->loop));
		watches[path] = {handle, callback};
		return error;
	}

	void Emit(const std::string &directory, const char *file, int status = 0) {
		const auto path = std::filesystem::absolute(directory).lexically_normal().string();
		const auto &watch = watches.at(path);
		watch.callback(watch.handle, file, UV_CHANGE, status);
	}
};

bool dispatchUntil(FileWatcher &watcher, std::set<std::string> &changed,
	const std::function<bool()> &done) {
	const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(4);
	while (std::chrono::steady_clock::now() < deadline) {
		watcher.Dispatch([&](const std::string &key) { changed.insert(key); });
		if (done()) {
			return true;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(5));
	}
	return false;
}
} // namespace

TEST_CASE("File watcher retains subscriptions and backs off after native startup errors") {
	NativeBackend native;
	native.error = UV_EMFILE;
	FileWatcher watcher([&](auto... args) { return native.Start(args...); });
	std::set<std::string> changed;
	REQUIRE(watcher.Watch("/tmp/sowa-watch-retry/a.lua", "res://a.lua"));
	REQUIRE(watcher.Watch("/tmp/sowa-watch-retry/a.lua", "alias://a.lua"));
	REQUIRE(watcher.Watch("/tmp/sowa-watch-retry/b.lua", "res://b.lua"));
	CHECK(native.attempts.size() == 1);
	REQUIRE(dispatchUntil(watcher, changed, [&] { return native.attempts.size() == 3; }));
	CHECK(changed.empty());
	CHECK(native.attempts[1] - native.attempts[0] >= 100);
	CHECK(native.attempts[2] - native.attempts[1] >= 200);

	native.error = 0;
	REQUIRE(dispatchUntil(watcher, changed, [&] { return changed.size() == 3; }));
	CHECK(native.attempts.size() == 4);
	CHECK(native.attempts[3] - native.attempts[2] >= 400);
	CHECK(changed == std::set<std::string>{"res://a.lua", "alias://a.lua", "res://b.lua"});

	changed.clear();
	native.Emit("/tmp/sowa-watch-retry", "a.lua");
	REQUIRE(dispatchUntil(watcher, changed, [&] { return changed.size() == 2; }));
	CHECK_FALSE(changed.contains("res://b.lua"));
	CHECK(native.attempts.size() == 4);
}

TEST_CASE("File watcher reattaches after callback errors and keeps healthy directories active") {
	NativeBackend native;
	FileWatcher watcher([&](auto... args) { return native.Start(args...); });
	std::set<std::string> changed;
	REQUIRE(watcher.Watch("/tmp/sowa-watch-failed/scene", "scene"));
	REQUIRE(watcher.Watch("/tmp/sowa-watch-healthy/texture", "texture"));

	native.error = UV_EMFILE;
	native.Emit("/tmp/sowa-watch-failed", nullptr, UV_EMFILE);
	native.Emit("/tmp/sowa-watch-healthy", "texture");
	REQUIRE(dispatchUntil(watcher, changed, [&] { return changed.contains("texture"); }));
	CHECK_FALSE(changed.contains("scene"));
	REQUIRE(dispatchUntil(watcher, changed, [&] { return native.attempts.size() >= 3; }));

	// Recovery invalidates files even if every change during the outage was lost.
	native.error = 0;
	REQUIRE(dispatchUntil(watcher, changed, [&] { return changed.contains("scene"); }));
	changed.clear();
	native.Emit("/tmp/sowa-watch-failed", "scene");
	REQUIRE(dispatchUntil(watcher, changed, [&] { return changed.contains("scene"); }));

	// Teardown must cancel a pending retry rather than keeping the event loop alive.
	native.Emit("/tmp/sowa-watch-failed", nullptr, UV_ENOSPC);
}

TEST_CASE("Native file watcher resumes notifications after startup and callback failures") {
	struct Directory {
		std::filesystem::path path =
			std::filesystem::temp_directory_path() /
			("sowa_watch_recovery_" +
				std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));

		Directory() {
			std::filesystem::create_directories(path);
		}

		~Directory() {
			std::filesystem::remove_all(path);
		}
	} directory;

	const auto file = directory.path / "scene";
	std::ofstream(file) << "initial";
	uv_fs_event_t *nativeHandle = nullptr;
	FileWatcher::NativeCallback nativeCallback = nullptr;
	int failures = 1;
	FileWatcher watcher([&](auto *handle, auto callback, const char *path, unsigned flags) -> int {
		nativeHandle = handle;
		nativeCallback = callback;
		if (failures > 0) {
			--failures;
			return UV_EMFILE;
		}
		return uv_fs_event_start(handle, callback, path, flags);
	});
	std::set<std::string> changed;
	REQUIRE(watcher.Watch(file, "scene"));
	REQUIRE(dispatchUntil(watcher, changed, [&] { return changed.contains("scene"); }));

	changed.clear();
	failures = 1;
	nativeCallback(nativeHandle, nullptr, 0, UV_EMFILE);
	std::ofstream(file) << "changed during outage";
	REQUIRE(dispatchUntil(watcher, changed, [&] { return changed.contains("scene"); }));

	// Recovery invalidation has been consumed; this requires a fresh OS event.
	changed.clear();
	std::ofstream(file) << "changed after recovery";
	REQUIRE(dispatchUntil(watcher, changed, [&] { return changed.contains("scene"); }));
}
#endif
