#include <doctest/doctest.h>

#include <filesystem>
#include <fstream>
#include <sstream>

#include "core/profiler.hpp"
#include "core/profiler_report.hpp"
#include "editor/profiler_panel.hpp"

using namespace sowa;

namespace {

void RecordFrames(Profiler &profiler, int count) {
	for (int i = 0; i < count; ++i) {
		profiler.BeginFrame();
		profiler.EndFrame();
	}
}

} // namespace

TEST_CASE("ResolveProfilerFrame follows the newest frame unless one is held") {
	Profiler profiler(3);
	CHECK(editor::ResolveProfilerFrame(profiler, 0) == 0);

	RecordFrames(profiler, 5); // holds serials 3, 4, 5
	CHECK(editor::ResolveProfilerFrame(profiler, 0) == 2);
	CHECK(editor::ResolveProfilerFrame(profiler, 4) == 1);
	CHECK(editor::ResolveProfilerFrame(profiler, 3) == 0);
	// A selection that scrolled out of the history falls back to the newest.
	CHECK(editor::ResolveProfilerFrame(profiler, 1) == 2);
}

TEST_CASE("ProfilerTracePath is a timestamped file in the project's .sowa folder") {
	std::tm time{};
	time.tm_year = 2026 - 1900;
	time.tm_mon = 9;
	time.tm_mday = 8;
	time.tm_hour = 21;
	time.tm_min = 30;
	time.tm_sec = 45;
	time.tm_isdst = -1;
	const std::time_t stamp = std::mktime(&time);

	const std::filesystem::path path = editor::ProfilerTracePath("/projects/game", stamp);
	CHECK(
		path == std::filesystem::path("/projects/game/.sowa/profiler/trace-20261008-213045.json"));
}

TEST_CASE("WriteProfilerTrace creates the folder and writes the trace") {
	const std::filesystem::path root =
		std::filesystem::temp_directory_path() / "sowa_profiler_trace_test";
	std::filesystem::remove_all(root);

	Profiler profiler;
	RecordFrames(profiler, 2);
	const std::filesystem::path path = root / ".sowa" / "profiler" / "trace.json";
	CHECK(editor::WriteProfilerTrace(profiler, path).empty());

	std::ifstream file(path, std::ios::binary);
	REQUIRE(file);
	std::stringstream contents;
	contents << file.rdbuf();
	CHECK(contents.str() == ToChromeTrace(profiler));

	// A directory where the file should be cannot be opened for writing.
	const std::filesystem::path directory = root / "occupied.json";
	std::filesystem::create_directories(directory);
	CHECK_FALSE(editor::WriteProfilerTrace(profiler, directory).empty());

	// A file where the folder should be cannot be written through.
	const std::filesystem::path blocked = root / "blocked";
	std::ofstream(blocked) << "x";
	CHECK_FALSE(editor::WriteProfilerTrace(profiler, blocked / "trace.json").empty());
	std::filesystem::remove_all(root);
}

TEST_CASE("A closed Profiler panel turns recording off") {
	Profiler &profiler = Profiler::Main();
	profiler.Clear();
	profiler.SetEnabled(true);

	editor::ProfilerPanel panel;
	bool open = false;
	panel.Draw(open, {});
	CHECK_FALSE(profiler.IsEnabled());
	profiler.BeginFrame();
	CHECK_FALSE(profiler.IsRecording());
	profiler.EndFrame();
	CHECK(profiler.FrameCount() == 0);
	profiler.SetEnabled(true);
}
