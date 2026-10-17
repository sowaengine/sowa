#include <doctest/doctest.h>

#include "core/string.hpp"

using namespace sowa;

TEST_CASE("String::Split") {
	auto parts = String::Split("a::b::c", "::");
	REQUIRE(parts.size() == 3);
	CHECK(parts[0] == "a");
	CHECK(parts[1] == "b");
	CHECK(parts[2] == "c");
}

TEST_CASE("String::Split with no delimiter present") {
	auto parts = String::Split("hello", ",");
	REQUIRE(parts.size() == 1);
	CHECK(parts[0] == "hello");
}

TEST_CASE("String::Split with empty input") {
	auto parts = String::Split("", ",");
	REQUIRE(parts.size() == 1);
	CHECK(parts[0] == "");
}

TEST_CASE("String::Trim variants") {
	CHECK(String::Trim("  hi  ") == "hi");
	CHECK(String::TrimLeft("  hi  ") == "hi  ");
	CHECK(String::TrimRight("  hi  ") == "  hi");
	CHECK(String::Trim("\t\nhi\n") == "hi");
	CHECK(String::Trim("") == "");
	CHECK(String::Trim("   ") == "");
}

TEST_CASE("String::StartsWith / EndsWith") {
	CHECK(String::StartsWith("foobar", "foo"));
	CHECK_FALSE(String::StartsWith("foobar", "bar"));
	CHECK(String::EndsWith("foobar", "bar"));
	CHECK_FALSE(String::EndsWith("foobar", "foo"));
	CHECK(String::StartsWith("foo", "foo"));
	CHECK_FALSE(String::StartsWith("fo", "foo"));
}

TEST_CASE("String::Join") {
	CHECK(String::Join({"a", "b", "c"}, ", ") == "a, b, c");
	CHECK(String::Join({"only"}, ", ") == "only");
	CHECK(String::Join({}, ", ") == "");
}
