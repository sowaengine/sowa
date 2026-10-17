#include <doctest/doctest.h>

#include "core/game_time.hpp"

using namespace sowa;

TEST_CASE("GetGameTime while stopped passes frame time without gameplay") {
	const GameTime time = GetGameTime(false, false, false, 0.25f, 0.1f);
	CHECK(time.delta == doctest::Approx(0.25f));
	CHECK_FALSE(time.running);
}

TEST_CASE("GetGameTime while running passes frame time with gameplay") {
	const GameTime time = GetGameTime(true, false, false, 0.25f, 0.1f);
	CHECK(time.delta == doctest::Approx(0.25f));
	CHECK(time.running);
}

TEST_CASE("GetGameTime while paused advances nothing") {
	const GameTime time = GetGameTime(true, true, false, 0.25f, 0.1f);
	CHECK(time.delta == 0.f);
	CHECK_FALSE(time.running);
}

TEST_CASE("GetGameTime on a step frame advances exactly the step delta") {
	const GameTime time = GetGameTime(true, true, true, 0.25f, 0.1f);
	CHECK(time.delta == doctest::Approx(0.1f));
	CHECK(time.running);
}
