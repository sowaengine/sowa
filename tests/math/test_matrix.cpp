#include <doctest/doctest.h>

#include "math/matrix.hpp"

using namespace sowa;

TEST_CASE("CalculateTransform2D / DecomposeTransform2D round-trip") {
	glm::mat4 m = Matrix::CalculateTransform2D(Vector2(10.f, 20.f), 30.f, Vector2(2.f, 3.f));

	Vector2 position;
	f32 rotation = 0.f;
	Vector2 scale;
	bool ok = Matrix::DecomposeTransform2D(m, &position, &rotation, &scale);

	REQUIRE(ok);
	CHECK(position.x == doctest::Approx(10.f));
	CHECK(position.y == doctest::Approx(20.f));
	CHECK(rotation == doctest::Approx(30.f));
	CHECK(scale.x == doctest::Approx(2.f));
	CHECK(scale.y == doctest::Approx(3.f));
}

TEST_CASE("CalculateTransform2D rotation is counter-clockwise around +Z, 0 degrees along +X") {
	// Rotating a point on +X by +90 degrees should land it on +Y (CCW, Y-up world).
	glm::mat4 m = Matrix::CalculateTransform2D(Vector2(0.f, 0.f), 90.f, Vector2(1.f, 1.f));
	glm::vec4 p = m * glm::vec4(1.f, 0.f, 0.f, 1.f);
	CHECK(p.x == doctest::Approx(0.f).epsilon(0.001));
	CHECK(p.y == doctest::Approx(1.f));
}

TEST_CASE("DecomposeTransform2D computes each requested output independently") {
	const glm::mat4 matrix =
		Matrix::CalculateTransform2D(Vector2(-7.f, 13.f), 27.f, Vector2(2.f, 3.f));
	Vector2 position;
	f32 rotation = 0.f;
	Vector2 scale;
	REQUIRE(Matrix::DecomposeTransform2D(matrix, &position, nullptr, nullptr));
	CHECK(position.x == doctest::Approx(-7.f));
	CHECK(position.y == doctest::Approx(13.f));
	REQUIRE(Matrix::DecomposeTransform2D(matrix, nullptr, &rotation, nullptr));
	CHECK(rotation == doctest::Approx(27.f));
	REQUIRE(Matrix::DecomposeTransform2D(matrix, nullptr, nullptr, &scale));
	CHECK(scale.x == doctest::Approx(2.f));
	CHECK(scale.y == doctest::Approx(3.f));
}

TEST_CASE("CalculateOrtho maps the given box to NDC [-1, 1]") {
	glm::mat4 m = Matrix::CalculateOrtho(0.f, 100.f, 0.f, 100.f);

	glm::vec4 origin = m * glm::vec4(0.f, 0.f, 0.f, 1.f);
	CHECK(origin.x == doctest::Approx(-1.f));
	CHECK(origin.y == doctest::Approx(-1.f));

	glm::vec4 farCorner = m * glm::vec4(100.f, 100.f, 0.f, 1.f);
	CHECK(farCorner.x == doctest::Approx(1.f));
	CHECK(farCorner.y == doctest::Approx(1.f));
}

TEST_CASE("CalculateTransform3D translates a point correctly") {
	glm::mat4 m = Matrix::CalculateTransform3D(Vector3(1.f, 2.f, 3.f), Vector3(0.f, 0.f, 0.f),
		Vector3(1.f, 1.f, 1.f));
	glm::vec4 p = m * glm::vec4(0.f, 0.f, 0.f, 1.f);
	CHECK(p.x == doctest::Approx(1.f));
	CHECK(p.y == doctest::Approx(2.f));
	CHECK(p.z == doctest::Approx(3.f));
}
