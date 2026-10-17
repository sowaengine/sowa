#include <doctest/doctest.h>

#include "math/vector3.hpp"

using namespace sowa;

TEST_CASE("Vector3 length and normalization") {
	Vector3 v(1.f, 2.f, 2.f);
	CHECK(v.Length() == doctest::Approx(3.f));
	CHECK(v.LengthSquared() == doctest::Approx(9.f));

	Vector3 n = v.Normalized();
	CHECK(n.Length() == doctest::Approx(1.f));
	CHECK(n.x == doctest::Approx(1.f / 3.f));
	CHECK(n.y == doctest::Approx(2.f / 3.f));
	CHECK(n.z == doctest::Approx(2.f / 3.f));
	CHECK(v.x == 1.f); // untouched
	CHECK(v.y == 2.f);
	CHECK(v.z == 2.f);
	Vector3 negative(-1.f, -2.f, -2.f);
	negative.Normalize();
	CHECK(negative.x == doctest::Approx(-1.f / 3.f));
	CHECK(negative.y == doctest::Approx(-2.f / 3.f));
	CHECK(negative.z == doctest::Approx(-2.f / 3.f));

	Vector3 zero;
	zero.Normalize();
	CHECK(zero.x == 0.f);
	CHECK(zero.y == 0.f);
	CHECK(zero.z == 0.f);
}

TEST_CASE("Vector3 distance") {
	Vector3 a(0.f, 0.f, 0.f);
	Vector3 b(1.f, 2.f, 2.f);
	CHECK(a.DistanceTo(b) == doctest::Approx(3.f));
}

TEST_CASE("Vector3 arithmetic operators") {
	Vector3 a(1.f, 2.f, 3.f);
	Vector3 b(4.f, 5.f, 6.f);

	CHECK((a + b) == Vector3(5.f, 7.f, 9.f));
	CHECK((b - a) == Vector3(3.f, 3.f, 3.f));
	CHECK((-a) == Vector3(-1.f, -2.f, -3.f));
	CHECK((a * b) == Vector3(4.f, 10.f, 18.f));
	CHECK((a * 2.f) == Vector3(2.f, 4.f, 6.f));
	CHECK((b / a) == Vector3(4.f, 2.5f, 2.f));
	CHECK((b / 2.f) == Vector3(2.f, 2.5f, 3.f));

	Vector3 c(1.f, 1.f, 1.f);
	c += a;
	CHECK(c == Vector3(2.f, 3.f, 4.f));
	c -= a;
	CHECK(c == Vector3(1.f, 1.f, 1.f));
	c *= 2.f;
	CHECK(c == Vector3(2.f, 2.f, 2.f));
	c /= 2.f;
	CHECK(c == Vector3(1.f, 1.f, 1.f));
}

TEST_CASE("Vector3 equality") {
	CHECK(Vector3(1.f, 2.f, 3.f) == Vector3(1.f, 2.f, 3.f));
	CHECK(Vector3(1.f, 2.f, 3.f) != Vector3(3.f, 2.f, 1.f));
}
