#include <doctest/doctest.h>

#include "math/color.hpp"

using namespace sowa;

TEST_CASE("Color::RGB converts u8 channels to 0..1 floats") {
	Color c = Color::RGB(255, 128, 0, 255);
	CHECK(c.r == doctest::Approx(1.f));
	CHECK(c.g == doctest::Approx(128.f / 255.f));
	CHECK(c.b == doctest::Approx(0.f));
	CHECK(c.a == doctest::Approx(1.f));
}

TEST_CASE("Color::HSV primary colors") {
	Color red = Color::HSV(0.f, 1.f, 1.f);
	CHECK(red.r == doctest::Approx(1.f));
	CHECK(red.g == doctest::Approx(0.f));
	CHECK(red.b == doctest::Approx(0.f));

	Color green = Color::HSV(120.f, 1.f, 1.f);
	CHECK(green.r == doctest::Approx(0.f));
	CHECK(green.g == doctest::Approx(1.f));
	CHECK(green.b == doctest::Approx(0.f));

	Color blue = Color::HSV(240.f, 1.f, 1.f);
	CHECK(blue.r == doctest::Approx(0.f));
	CHECK(blue.g == doctest::Approx(0.f));
	CHECK(blue.b == doctest::Approx(1.f));
}

TEST_CASE("Color::HSV handles out-of-range/negative hue") {
	Color wrapped = Color::HSV(480.f, 1.f, 1.f);
	CHECK(wrapped.r == doctest::Approx(0.f));
	CHECK(wrapped.g == doctest::Approx(1.f));
	CHECK(wrapped.b == doctest::Approx(0.f));

	Color negative = Color::HSV(-240.f, 1.f, 1.f);
	CHECK(negative.r == doctest::Approx(0.f));
	CHECK(negative.g == doctest::Approx(1.f));
	CHECK(negative.b == doctest::Approx(0.f));
}

TEST_CASE("Color::HSV zero saturation is grayscale") {
	Color gray = Color::HSV(0.f, 0.f, 0.5f);
	CHECK(gray.r == doctest::Approx(0.5f));
	CHECK(gray.g == doctest::Approx(0.5f));
	CHECK(gray.b == doctest::Approx(0.5f));
}

TEST_CASE("Color equality") {
	CHECK(Color(1.f, 0.f, 0.f, 1.f) == Color(1.f, 0.f, 0.f, 1.f));
	CHECK(Color(1.f, 0.f, 0.f, 1.f) != Color(0.f, 1.f, 0.f, 1.f));
}
