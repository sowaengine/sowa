#include <cmath>

#include <doctest/doctest.h>

#include "math/easing.hpp"
#include "math/math.hpp"

using namespace sowa;

TEST_CASE("Ease endpoints are 0 and 1 for every named curve") {
	for (i32 i = 0; i < static_cast<i32>(Easing::Count); ++i) {
		const Easing easing = static_cast<Easing>(i);
		CAPTURE(i);
		CHECK(Math::Ease(0.f, easing) == doctest::Approx(0.f));
		CHECK(Math::Ease(1.f, easing) == doctest::Approx(1.f));
	}
}

TEST_CASE("Ease Linear is identity") {
	CHECK(Math::Ease(0.3f, Easing::Linear) == doctest::Approx(0.3f));
	CHECK(Math::Ease(0.8f, Easing::Linear) == doctest::Approx(0.8f));
}

TEST_CASE("Ease family midpoints differ from linear") {
	CHECK(Math::Ease(0.5f, Easing::SineIn) == doctest::Approx(1.f - std::cos(Math::Pi * 0.25f)));
	CHECK(Math::Ease(0.5f, Easing::SineOut) == doctest::Approx(std::sin(Math::Pi * 0.25f)));
	CHECK(Math::Ease(0.5f, Easing::CubicIn) == doctest::Approx(0.125f));
	CHECK(Math::Ease(0.5f, Easing::QuintIn) == doctest::Approx(0.03125f));
	CHECK(Math::Ease(0.5f, Easing::CircIn) == doctest::Approx(1.f - std::sqrt(0.75f)));
	CHECK(Math::Ease(0.5f, Easing::QuadIn) == doctest::Approx(0.25f));
	CHECK(Math::Ease(0.5f, Easing::QuadOut) == doctest::Approx(0.75f));
	CHECK(Math::Ease(0.5f, Easing::QuartIn) == doctest::Approx(0.0625f));
	CHECK(Math::Ease(0.5f, Easing::ExpoIn) == doctest::Approx(0.03125f));
	CHECK(Math::Ease(0.5f, Easing::BounceOut) == doctest::Approx(0.765625f));
}

TEST_CASE("Ease QuadOut is 1 minus QuadIn of the remainder") {
	CHECK(Math::Ease(0.3f, Easing::QuadOut) ==
		  doctest::Approx(1.f - Math::Ease(0.7f, Easing::QuadIn)));
}

TEST_CASE("Ease unknown value is linear") {
	CHECK(Math::Ease(0.4f, static_cast<Easing>(999)) == doctest::Approx(0.4f));
	CHECK(Math::Ease(0.4f, Easing::Count) == doctest::Approx(0.4f));
}

TEST_CASE("Ease BackIn undershoots below 0") {
	CHECK(Math::Ease(0.5f, Easing::BackIn) < 0.f);
}

TEST_CASE("Math::Lerp with Easing eases t first") {
	CHECK(Math::Lerp(0.f, 10.f, 0.5f, Easing::QuadIn) == doctest::Approx(2.5f));
	CHECK(Math::Lerp(0.f, 10.f, 0.5f, Easing::Linear) == doctest::Approx(5.f));
	Vector2 v = Math::Lerp(Vector2(0.f, 0.f), Vector2(10.f, 0.f), 0.5f, Easing::QuadOut);
	CHECK(v.x == doctest::Approx(7.5f));
}

TEST_CASE("Math::TweenMap remaps 0..1") {
	CHECK(Math::TweenMap(0.f, 0.f, 100.f) == doctest::Approx(0.f));
	CHECK(Math::TweenMap(0.5f, 0.f, 100.f) == doctest::Approx(50.f));
	CHECK(Math::TweenMap(1.f, 10.f, 20.f) == doctest::Approx(20.f));
}
