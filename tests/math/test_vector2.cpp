#include <doctest/doctest.h>

#include "math/vector2.hpp"

using namespace sowa;

TEST_CASE("Vector2 length and normalization") {
	Vector2 v(3.f, 4.f);
	CHECK(v.Length() == doctest::Approx(5.f));
	CHECK(v.LengthSquared() == doctest::Approx(25.f));

	Vector2 n = v.Normalized();
	CHECK(n.Length() == doctest::Approx(1.f));
	CHECK(n.x == doctest::Approx(0.6f));
	CHECK(n.y == doctest::Approx(0.8f));
	// Normalized() must not mutate the original.
	CHECK(v.x == 3.f);
	CHECK(v.y == 4.f);
	Vector2 negative(-3.f, -4.f);
	negative.Normalize();
	CHECK(negative.x == doctest::Approx(-0.6f));
	CHECK(negative.y == doctest::Approx(-0.8f));

	Vector2 zero;
	zero.Normalize(); // must not divide by zero
	CHECK(zero.x == 0.f);
	CHECK(zero.y == 0.f);
}

TEST_CASE("Vector2 distance") {
	Vector2 a(0.f, 0.f);
	Vector2 b(3.f, 4.f);
	CHECK(a.DistanceTo(b) == doctest::Approx(5.f));
	CHECK(b.DistanceTo(a) == doctest::Approx(5.f));
}

TEST_CASE("Vector2 arithmetic operators") {
	Vector2 a(1.f, 2.f);
	Vector2 b(3.f, 4.f);

	CHECK((a + b) == Vector2(4.f, 6.f));
	CHECK((b - a) == Vector2(2.f, 2.f));
	CHECK((-a) == Vector2(-1.f, -2.f));
	CHECK((a * b) == Vector2(3.f, 8.f));
	CHECK((a * 2.f) == Vector2(2.f, 4.f));
	CHECK((b / a) == Vector2(3.f, 2.f));
	CHECK((b / 2.f) == Vector2(1.5f, 2.f));

	Vector2 c(1.f, 1.f);
	c += a;
	CHECK(c == Vector2(2.f, 3.f));
	c -= a;
	CHECK(c == Vector2(1.f, 1.f));
	c *= 2.f;
	CHECK(c == Vector2(2.f, 2.f));
	c /= 2.f;
	CHECK(c == Vector2(1.f, 1.f));
}

TEST_CASE("Vector2 equality") {
	CHECK(Vector2(1.f, 2.f) == Vector2(1.f, 2.f));
	CHECK(Vector2(1.f, 2.f) != Vector2(2.f, 1.f));
}
