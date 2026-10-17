#include <doctest/doctest.h>

#include "math/rect.hpp"
#include "math/vector2.hpp"

using namespace sowa;

TEST_CASE("Rect accessors") {
	Rect r(1.f, 2.f, 10.f, 20.f);
	CHECK(r.Left() == 1.f);
	CHECK(r.Right() == 11.f);
	CHECK(r.Bottom() == 2.f);
	CHECK(r.Top() == 22.f);
}

TEST_CASE("Rect Contains") {
	Rect r(0.f, 0.f, 10.f, 10.f);
	CHECK(r.Contains(Vector2(5.f, 5.f)));
	CHECK(r.Contains(Vector2(0.f, 0.f)));
	CHECK(r.Contains(Vector2(10.f, 10.f)));
	CHECK_FALSE(r.Contains(Vector2(11.f, 5.f)));
	CHECK_FALSE(r.Contains(Vector2(5.f, -1.f)));
}

TEST_CASE("Rect Intersects") {
	Rect a(0.f, 0.f, 10.f, 10.f);
	Rect overlapping(5.f, 5.f, 10.f, 10.f);
	Rect touchingEdge(10.f, 0.f, 5.f, 5.f);
	Rect disjoint(100.f, 100.f, 5.f, 5.f);

	CHECK(a.Intersects(overlapping));
	CHECK(a.Intersects(touchingEdge));
	CHECK_FALSE(a.Intersects(disjoint));
}

TEST_CASE("Rect MapPoint") {
	Rect from(0.f, 0.f, 10.f, 10.f);
	Rect to(0.f, 0.f, 100.f, 100.f);

	Vector2 mapped = from.MapPoint(Vector2(5.f, 5.f), to);
	CHECK(mapped.x == doctest::Approx(50.f));
	CHECK(mapped.y == doctest::Approx(50.f));

	Vector2 corner = from.MapPoint(Vector2(0.f, 0.f), to);
	CHECK(corner.x == doctest::Approx(0.f));
	CHECK(corner.y == doctest::Approx(0.f));
}
