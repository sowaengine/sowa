#include <doctest/doctest.h>

#include "render/viewport_scale.hpp"

using namespace sowa;

TEST_CASE("CalculatePresentRect Stretch fills the window") {
	const Rect r = CalculatePresentRect(1920, 1080, 1280, 720,
										ViewportScaleMode::Stretch);
	CHECK(r.x == doctest::Approx(0.f));
	CHECK(r.y == doctest::Approx(0.f));
	CHECK(r.w == doctest::Approx(1280.f));
	CHECK(r.h == doctest::Approx(720.f));
}

TEST_CASE("CalculatePresentRect KeepAspect letterboxes a wide window") {
	// Window wider than 16:9 content → pillarbox.
	const Rect r = CalculatePresentRect(1920, 1080, 1600, 720,
										ViewportScaleMode::KeepAspect);
	CHECK(r.h == doctest::Approx(720.f));
	CHECK(r.w == doctest::Approx(720.f * 16.f / 9.f));
	CHECK(r.x == doctest::Approx((1600.f - r.w) * 0.5f));
	CHECK(r.y == doctest::Approx(0.f));
}

TEST_CASE("CalculatePresentRect KeepAspect pillarboxes a tall window") {
	const Rect r = CalculatePresentRect(1920, 1080, 800, 900,
										ViewportScaleMode::KeepAspect);
	CHECK(r.w == doctest::Approx(800.f));
	CHECK(r.h == doctest::Approx(800.f * 9.f / 16.f));
	CHECK(r.x == doctest::Approx(0.f));
	CHECK(r.y == doctest::Approx((900.f - r.h) * 0.5f));
}

TEST_CASE("CalculatePresentRect KeepWidth matches window width") {
	const Rect r = CalculatePresentRect(100, 50, 200, 200,
										ViewportScaleMode::KeepWidth);
	CHECK(r.w == doctest::Approx(200.f));
	CHECK(r.h == doctest::Approx(100.f));
	CHECK(r.x == doctest::Approx(0.f));
	CHECK(r.y == doctest::Approx(50.f));
}

TEST_CASE("CalculatePresentRect KeepHeight matches window height") {
	const Rect r = CalculatePresentRect(100, 50, 200, 200,
										ViewportScaleMode::KeepHeight);
	CHECK(r.h == doctest::Approx(200.f));
	CHECK(r.w == doctest::Approx(400.f));
	CHECK(r.x == doctest::Approx(-100.f));
	CHECK(r.y == doctest::Approx(0.f));
}

TEST_CASE("CalculatePresentRect NoResize is 1:1 centered") {
	const Rect r =
		CalculatePresentRect(100, 50, 300, 200, ViewportScaleMode::NoResize);
	CHECK(r.w == doctest::Approx(100.f));
	CHECK(r.h == doctest::Approx(50.f));
	CHECK(r.x == doctest::Approx(100.f));
	CHECK(r.y == doctest::Approx(75.f));
}

TEST_CASE("TryParseViewportScaleMode") {
	ViewportScaleMode mode = ViewportScaleMode::Stretch;
	CHECK(TryParseViewportScaleMode("KeepAspect", mode));
	CHECK(mode == ViewportScaleMode::KeepAspect);
	CHECK_FALSE(TryParseViewportScaleMode("Nope", mode));
	CHECK(mode == ViewportScaleMode::KeepAspect);
}

TEST_CASE("MapMouseToViewport Stretch maps center") {
	const Rect present(0.f, 0.f, 800.f, 600.f);
	const Vector2 mapped =
		MapMouseToViewport(Vector2(400.f, 300.f), Vector2(800.f, 600.f),
						   Vector2(800.f, 600.f), present, Vector2(1920.f, 1080.f));
	CHECK(mapped.x == doctest::Approx(960.f));
	CHECK(mapped.y == doctest::Approx(540.f));
}

TEST_CASE("MapMouseToViewport handles retina content scale") {
	// 2x framebuffer vs content size; cursor is in content coords.
	const Rect present(0.f, 0.f, 1600.f, 1200.f);
	const Vector2 mapped =
		MapMouseToViewport(Vector2(400.f, 300.f), Vector2(800.f, 600.f),
						   Vector2(1600.f, 1200.f), present,
						   Vector2(1920.f, 1080.f));
	CHECK(mapped.x == doctest::Approx(960.f));
	CHECK(mapped.y == doctest::Approx(540.f));
}

TEST_CASE("ViewportPointToPixel preserves bottom-left viewport coordinates") {
	const auto pixel = ViewportPointToPixel(Vector2(12.8f, 34.2f), 100, 80);
	REQUIRE(pixel);
	CHECK(pixel->x == 12);
	CHECK(pixel->y == 34);
}

TEST_CASE("ViewportPointToPixel clamps exact top and right edges") {
	const auto pixel = ViewportPointToPixel(Vector2(100.f, 80.f), 100, 80);
	REQUIRE(pixel);
	CHECK(pixel->x == 99);
	CHECK(pixel->y == 79);
}

TEST_CASE("ViewportPointToPixel rejects points outside the viewport") {
	CHECK_FALSE(ViewportPointToPixel(Vector2(-0.1f, 20.f), 100, 80));
	CHECK_FALSE(ViewportPointToPixel(Vector2(20.f, 80.1f), 100, 80));
	CHECK_FALSE(ViewportPointToPixel(Vector2(20.f, 20.f), 0, 80));
}
