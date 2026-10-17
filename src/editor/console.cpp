#include "editor/console.hpp"

#include "imgui.h"

namespace sowa::editor {

Color ColorForSeverity(Log::Severity severity) {
	switch (severity) {
	case Log::Severity::Trace:
		return Color::RGB(102, 187, 106);
	case Log::Severity::Info:
		return Color::RGB(128, 212, 226);
	case Log::Severity::Warn:
		return Color::RGB(255, 193, 7);
	case Log::Severity::Error:
		return Color::RGB(239, 83, 80);
	}
	return Color::RGB(200, 200, 200);
}

bool PassesFilter(Log::Severity severity, const LogFilter &filter) {
	switch (severity) {
	case Log::Severity::Trace:
		return filter.trace;
	case Log::Severity::Info:
		return filter.info;
	case Log::Severity::Warn:
		return filter.warn;
	case Log::Severity::Error:
		return filter.error;
	}
	return false;
}

std::vector<ConsoleRow> BuildConsoleRows(const std::deque<Log::Message> &messages,
	const LogFilter &filter) {
	std::vector<ConsoleRow> rows;
	rows.reserve(messages.size());
	for (const auto &message : messages) {
		if (!PassesFilter(message.severity, filter)) {
			continue;
		}
		std::string_view remaining = message.text;
		for (;;) {
			const auto end = remaining.find_first_of("\r\n");
			rows.push_back({remaining.substr(0, end), message.severity});
			if (end == std::string_view::npos) {
				break;
			}
			const bool crlf =
				remaining[end] == '\r' && end + 1 < remaining.size() && remaining[end + 1] == '\n';
			remaining.remove_prefix(end + (crlf ? 2 : 1));
		}
	}
	return rows;
}

void DrawConsoleRows(const std::vector<ConsoleRow> &rows) {
	ImGuiListClipper clipper;
	clipper.Begin(static_cast<int>(rows.size()), ImGui::GetTextLineHeightWithSpacing());
	while (clipper.Step()) {
		for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i) {
			const auto &row = rows[static_cast<std::size_t>(i)];
			if (row.text.empty()) {
				ImGui::Dummy(ImVec2(0.f, ImGui::GetTextLineHeight()));
				continue;
			}
			const Color color = ColorForSeverity(row.severity);
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(color.r, color.g, color.b, color.a));
			ImGui::TextUnformatted(row.text.data(), row.text.data() + row.text.size());
			ImGui::PopStyleColor();
		}
	}
}

bool ShouldStickToBottom(bool autoScroll, bool wasAtBottom) {
	return autoScroll && wasAtBottom;
}

} // namespace sowa::editor
