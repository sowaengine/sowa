#include <doctest/doctest.h>

#include "core/scope_exit.hpp"
#include "editor/console.hpp"
#include "imgui.h"
#include "math/color.hpp"

using namespace sowa;

TEST_CASE("ColorForSeverity uses a distinct hue per severity") {
	const Color trace = editor::ColorForSeverity(Log::Severity::Trace);
	CHECK(trace.g > trace.r);
	CHECK(trace.g > trace.b);

	const Color info = editor::ColorForSeverity(Log::Severity::Info);
	CHECK(info.b > info.r);
	CHECK(info.g > info.r);

	const Color warn = editor::ColorForSeverity(Log::Severity::Warn);
	CHECK(warn.r > 0.7f);
	CHECK(warn.g > 0.5f);
	CHECK(warn.b < 0.4f);

	const Color error = editor::ColorForSeverity(Log::Severity::Error);
	CHECK(error.r > error.g);
	CHECK(error.r > error.b);

	CHECK(trace != info);
	CHECK(info != warn);
	CHECK(warn != error);
}

TEST_CASE("PassesFilter hides unchecked severities") {
	editor::LogFilter filter;
	filter.trace = false;
	filter.info = false;
	filter.warn = true;
	filter.error = false;

	CHECK(editor::PassesFilter(Log::Severity::Warn, filter));
	CHECK_FALSE(editor::PassesFilter(Log::Severity::Trace, filter));
	CHECK_FALSE(editor::PassesFilter(Log::Severity::Info, filter));
	CHECK_FALSE(editor::PassesFilter(Log::Severity::Error, filter));
}

TEST_CASE("PassesFilter with defaults shows every severity") {
	const editor::LogFilter filter;
	CHECK(editor::PassesFilter(Log::Severity::Trace, filter));
	CHECK(editor::PassesFilter(Log::Severity::Info, filter));
	CHECK(editor::PassesFilter(Log::Severity::Warn, filter));
	CHECK(editor::PassesFilter(Log::Severity::Error, filter));
}

TEST_CASE("ShouldStickToBottom only when autoscroll is on and already at bottom") {
	CHECK(editor::ShouldStickToBottom(true, true));
	CHECK_FALSE(editor::ShouldStickToBottom(true, false));
	CHECK_FALSE(editor::ShouldStickToBottom(false, true));
	CHECK_FALSE(editor::ShouldStickToBottom(false, false));
}

TEST_CASE("Console splits multiline messages into uniform rows with severity filtering") {
	const std::deque<Log::Message> messages = {
		{"first\nsecond\n\nfourth\n", Log::Severity::Error},
		{"windows\r\nline\rlast", Log::Severity::Info},
		{"", Log::Severity::Warn},
	};
	const auto rows = editor::BuildConsoleRows(messages, {});
	REQUIRE(rows.size() == 9);
	CHECK(rows[0].text == "first");
	CHECK(rows[1].text == "second");
	CHECK(rows[2].text.empty());
	CHECK(rows[3].text == "fourth");
	CHECK(rows[4].text.empty());
	CHECK(rows[5].text == "windows");
	CHECK(rows[6].text == "line");
	CHECK(rows[7].text == "last");
	CHECK(rows[8].text.empty());
	for (std::size_t i = 0; i < 5; ++i) {
		CHECK(rows[i].severity == Log::Severity::Error);
	}
	editor::LogFilter filter;
	filter.error = false;
	const auto filtered = editor::BuildConsoleRows(messages, filter);
	REQUIRE(filtered.size() == 4);
	CHECK(filtered.front().text == "windows");
	CHECK(filtered.back().severity == Log::Severity::Warn);
}

TEST_CASE("Console clipper preserves multiline and blank-row heights while scrolling") {
	ImGui::CreateContext();
	const auto cleanup = MakeScopeExit([] { ImGui::DestroyContext(); });
	auto &io = ImGui::GetIO();
	io.IniFilename = nullptr;
	io.DisplaySize = {400, 240};
	io.DeltaTime = 1.f / 60.f;
	unsigned char *pixels;
	int width, height;
	io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
	std::deque<Log::Message> messages;
	for (int i = 0; i < 100; ++i) {
		messages.push_back({"error\n\nstack frame", Log::Severity::Error});
	}
	const auto rows = editor::BuildConsoleRows(messages, {});
	REQUIRE(rows.size() == 300);
	for (int frame = 0; frame < 3; ++frame) {
		ImGui::NewFrame();
		ImGui::SetNextWindowPos({0, 0});
		ImGui::SetNextWindowSize({400, 240});
		ImGui::Begin("Console rows", nullptr, ImGuiWindowFlags_NoTitleBar);
		if (frame == 1) {
			ImGui::SetScrollY(1000);
		}
		const float start = ImGui::GetCursorPosY();
		editor::DrawConsoleRows(rows);
		CHECK(ImGui::GetCursorPosY() - start ==
			  doctest::Approx(300 * ImGui::GetTextLineHeightWithSpacing()));
		ImGui::End();
		ImGui::Render();
	}
}
