#pragma once

#include <string_view>
#include <vector>

#include "core/log.hpp"
#include "math/color.hpp"

namespace sowa::editor {

// Per-severity visibility for the Console panel. All on by default so a
// new session shows every line Log has captured.
struct LogFilter {
	bool trace = true;
	bool info = true;
	bool warn = true;
	bool error = true;
};

// Hue matches the ANSI colors Log already prints to the terminal
// (green / cyan / yellow / red). The panel colors the same buffer, not a
// second log.
Color ColorForSeverity(Log::Severity severity);

bool PassesFilter(Log::Severity severity, const LogFilter &filter);

struct ConsoleRow {
	std::string_view text;
	Log::Severity severity;
};

// Borrowed views, valid until the log buffer changes. Filtering applies to
// whole messages; clipping operates on individual equal-height display rows.
std::vector<ConsoleRow> BuildConsoleRows(const std::deque<Log::Message> &messages,
	const LogFilter &filter);
void DrawConsoleRows(const std::vector<ConsoleRow> &rows);

// Stick to the last line only when autoscroll is on *and* the child was
// already at the bottom. A user who scrolls up must be able to read.
bool ShouldStickToBottom(bool autoScroll, bool wasAtBottom);

} // namespace sowa::editor
