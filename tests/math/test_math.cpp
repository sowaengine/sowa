#include <doctest/doctest.h>

#include "math/math.hpp"

using namespace sowa;

TEST_CASE("Math::Clamp") {
	CHECK(Math::Clamp(5, 0, 10) == 5);
	CHECK(Math::Clamp(-5, 0, 10) == 0);
	CHECK(Math::Clamp(15, 0, 10) == 10);
	CHECK(Math::Clamp(5, 10, 0) == 5); // swapped bounds still work
}

TEST_CASE("Math::Lerp") {
	CHECK(Math::Lerp(0.f, 10.f, 0.5f) == doctest::Approx(5.f));
	CHECK(Math::Lerp(0.f, 10.f, 0.f) == doctest::Approx(0.f));
	CHECK(Math::Lerp(0.f, 10.f, 1.f) == doctest::Approx(10.f));

	Vector2 lerped2 = Math::Lerp(Vector2(0.f, 0.f), Vector2(10.f, 20.f), 0.5f);
	CHECK(lerped2 == Vector2(5.f, 10.f));

	Vector3 lerped3 = Math::Lerp(Vector3(0.f, 0.f, 0.f), Vector3(10.f, 20.f, 30.f), 0.5f);
	CHECK(lerped3 == Vector3(5.f, 10.f, 15.f));

	Color lerpedColor = Math::Lerp(Color(0.f, 0.f, 0.f, 0.f), Color(1.f, 1.f, 1.f, 1.f), 0.5f);
	CHECK(lerpedColor.r == doctest::Approx(0.5f));
	CHECK(lerpedColor.a == doctest::Approx(0.5f));
}

TEST_CASE("Math::Map") {
	CHECK(Math::Map(5.f, 0.f, 10.f, 0.f, 100.f) == doctest::Approx(50.f));
	CHECK(Math::Map(0.f, 0.f, 10.f, 0.f, 100.f) == doctest::Approx(0.f));
	CHECK(Math::Map(10.f, 0.f, 10.f, 0.f, 100.f) == doctest::Approx(100.f));
	CHECK(Math::Map(5.f, 0.f, 10.f, 100.f, 0.f) == doctest::Approx(50.f));
}
