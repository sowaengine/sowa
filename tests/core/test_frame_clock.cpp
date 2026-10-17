#include <doctest/doctest.h>

#include <memory>

#include "core/frame_clock.hpp"

using namespace sowa;

TEST_CASE("FrameClock measures time between ticks") {
	auto now = std::make_shared<f64>(10.0);
	FrameClock clock([now] { return *now; });
	*now = 10.25;
	CHECK(clock.Tick() == doctest::Approx(0.25f));
	*now = 10.625;
	CHECK(clock.Tick() == doctest::Approx(0.375f));
	CHECK(clock.Tick() == 0.f);
}

TEST_CASE("FrameClock::Rebase leaves time spent outside the loop out of the next delta") {
	auto now = std::make_shared<f64>(0.0);
	FrameClock clock([now] { return *now; });
	*now = 0.016;
	clock.Tick();
	// Held at a breakpoint for 30 seconds mid-frame, then resumed.
	*now = 30.016;
	clock.Rebase();
	*now = 30.032;
	CHECK(clock.Tick() == doctest::Approx(0.016f));
}
