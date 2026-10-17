#pragma once

#include <cstddef>
#include <ctime>
#include <filesystem>
#include <string>

#include "core/profiler.hpp"
#include "core/types.hpp"

namespace sowa::editor {

// Index of the held frame with serial frameIndex, or of the newest frame
// when that one is not held (0 means "follow the newest"). FrameCount() when
// nothing is held.
std::size_t ResolveProfilerFrame(const Profiler &profiler, u64 frameIndex);

// Where an exported trace goes: the project's editor-workspace folder, which
// new projects already ignore in version control and exports never package.
// e.g. <project>/.sowa/profiler/trace-20261008-213045.json
std::filesystem::path ProfilerTracePath(const std::filesystem::path &projectRoot, std::time_t now);

// Writes Profiler::Main()'s history as a Chrome trace to path, creating its
// directory. Empty on success, otherwise the reason.
std::string WriteProfilerTrace(const Profiler &profiler, const std::filesystem::path &path);

// Profiler window. Records only while open.
class ProfilerPanel {
  public:
	// Call every frame, open or not: it also switches recording.
	void Draw(bool &open, const std::filesystem::path &projectRoot);

  private:
	void drawToolbar(Profiler &profiler, const std::filesystem::path &projectRoot);
	void drawFrameGraph(Profiler &profiler, std::size_t selected);
	void drawFrameTree(const Profiler &profiler, const ProfileFrame &frame);
	void drawScopeTable(const Profiler &profiler);
	void drawCounterTable(const Profiler &profiler, const ProfileFrame &frame);

	// Serial of the frame shown in detail; 0 follows the newest frame.
	u64 _selectedFrame = 0;
	std::string _exportStatus;
	std::filesystem::path _exportedPath;
	bool _exportFailed = false;
};

} // namespace sowa::editor
