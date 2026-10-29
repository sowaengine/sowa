#include <doctest/doctest.h>

#include <string>

#include "core/log.hpp"

using namespace sowa;

TEST_CASE("Log ring buffer respects MaxLines") {
	std::size_t originalMax = Log::MaxLines();
	Log::Clear();
	Log::SetMaxLines(2);

	Log::Info("one");
	Log::Warn("two");
	Log::Error("three");

	REQUIRE(Log::GetLines().size() == 2);
	CHECK(Log::GetLines().front().severity == Log::Severity::Warn);
	CHECK(Log::GetLines().back().severity == Log::Severity::Error);

	Log::SetMaxLines(originalMax);
	Log::Clear();
}

TEST_CASE("Log::Clear empties the buffer") {
	Log::Info("hello");
	Log::Clear();
	CHECK(Log::GetLines().empty());
}

TEST_CASE("Log messages contain the formatted text") {
	Log::Clear();
	Log::Info("value={}", 42);
	REQUIRE(Log::GetLines().size() == 1);
	CHECK(Log::GetLines().back().text.find("value=42") != std::string::npos);
}

TEST_CASE("SeverityToString covers all severities") {
	CHECK(std::string(Log::SeverityToString(Log::Severity::Trace)) == "TRACE");
	CHECK(std::string(Log::SeverityToString(Log::Severity::Info)) == "INFO");
	CHECK(std::string(Log::SeverityToString(Log::Severity::Warn)) == "WARN");
	CHECK(std::string(Log::SeverityToString(Log::Severity::Error)) == "ERROR");
}

TEST_CASE("repeated runtime errors retain one diagnostic and count every failure") {
	Log::Clear();
	const std::size_t before = Log::ErrorCount();
	for (int i = 0; i < 20; ++i) {
		Log::Error("repeat failure");
	}
	REQUIRE(Log::GetLines().size() == 1);
	CHECK(Log::GetLines().back().repetitions == 20);
	CHECK(Log::ErrorCount() == before + 20);
	Log::Clear();
	CHECK(Log::ErrorCount() == before + 20);
}
