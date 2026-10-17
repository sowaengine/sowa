#include <doctest/doctest.h>

#include "ecs/components/camera2d.hpp"
#include "ecs/components/transform2d.hpp"
#include "ecs/world.hpp"
#include "editor/camera.hpp"
#include "editor/viewport.hpp"

using namespace sowa;

TEST_CASE("FitGameImage pillarboxes a wide panel") {
	// 16:9 game in a wider panel → side bars, full height.
	const Rect r = editor::FitGameImage(Vector2(1920.f, 1080.f), Vector2(1600.f, 720.f));
	CHECK(r.h == doctest::Approx(720.f));
	CHECK(r.w == doctest::Approx(720.f * 16.f / 9.f));
	CHECK(r.x == doctest::Approx((1600.f - r.w) * 0.5f));
	CHECK(r.y == doctest::Approx(0.f));
	CHECK(r.w < 1600.f);
}

TEST_CASE("FitGameImage letterboxes a tall panel") {
	const Rect r = editor::FitGameImage(Vector2(1920.f, 1080.f), Vector2(800.f, 900.f));
	CHECK(r.w == doctest::Approx(800.f));
	CHECK(r.h == doctest::Approx(800.f * 9.f / 16.f));
	CHECK(r.x == doctest::Approx(0.f));
	CHECK(r.y == doctest::Approx((900.f - r.h) * 0.5f));
	CHECK(r.h < 900.f);
}

TEST_CASE("GameImageToPresentRect flips Y into framebuffer space") {
	const Rect image(100.f, 80.f, 400.f, 300.f);
	const Rect present = editor::GameImageToPresentRect(
		image, Vector2(800.f, 600.f), Vector2(800.f, 600.f));
	CHECK(present.x == doctest::Approx(100.f));
	CHECK(present.w == doctest::Approx(400.f));
	CHECK(present.h == doctest::Approx(300.f));
	// Content Y-down 80 with height 300 → fb Y-up 600 - 380 = 220.
	CHECK(present.y == doctest::Approx(220.f));
}

TEST_CASE("GameImageToPresentRect scales for a 2x framebuffer") {
	const Rect image(100.f, 80.f, 400.f, 300.f);
	const Rect present = editor::GameImageToPresentRect(
		image, Vector2(800.f, 600.f), Vector2(1600.f, 1200.f));
	CHECK(present.x == doctest::Approx(200.f));
	CHECK(present.w == doctest::Approx(800.f));
	CHECK(present.h == doctest::Approx(600.f));
	CHECK(present.y == doctest::Approx(440.f));
}

TEST_CASE("ViewportWorldCorners recovers the blank project window bounds") {
	const Vector2 viewport(800.f, 600.f);
	const auto corners =
		editor::ViewportWorldCorners(CalculateBlankViewProjection(viewport));
	CHECK(corners[0] == Vector2(0.f, 0.f));
	CHECK(corners[1] == Vector2(800.f, 0.f));
	CHECK(corners[2] == Vector2(800.f, 600.f));
	CHECK(corners[3] == Vector2(0.f, 600.f));
}

TEST_CASE("ViewportWorldCorners includes Camera2D zoom and rotation") {
	World world;
	Entity camera = world.CreateEntity();
	Transform2D transform;
	transform.position = Vector2(100.f, 50.f);
	transform.rotation = 90.f;
	transform.scale = Vector2(2.f, 2.f);
	camera.Add(transform);
	Camera2D settings;
	settings.rotatable = true;
	camera.Add(settings);

	const auto corners = editor::ViewportWorldCorners(
		CalculateViewProjection(camera, Vector2(800.f, 600.f)));
	CHECK(corners[0].x == doctest::Approx(700.f).epsilon(0.001));
	CHECK(corners[0].y == doctest::Approx(-750.f).epsilon(0.001));
	CHECK(corners[1].x == doctest::Approx(700.f).epsilon(0.001));
	CHECK(corners[1].y == doctest::Approx(850.f).epsilon(0.001));
	CHECK(corners[2].x == doctest::Approx(-500.f).epsilon(0.001));
	CHECK(corners[2].y == doctest::Approx(850.f).epsilon(0.001));
	CHECK(corners[3].x == doctest::Approx(-500.f).epsilon(0.001));
	CHECK(corners[3].y == doctest::Approx(-750.f).epsilon(0.001));
}

TEST_CASE("WorldToViewportImage includes editor pan and zoom") {
	editor::EditorCamera camera;
	camera.position = Vector2(100.f, 50.f);
	camera.zoom = 2.f;
	const glm::mat4 matrix =
		editor::ViewProjection(camera, Vector2(800.f, 600.f));
	const Rect image(10.f, 20.f, 400.f, 300.f);

	const Vector2 centre =
		editor::WorldToViewportImage(matrix, camera.position, image);
	CHECK(centre.x == doctest::Approx(210.f));
	CHECK(centre.y == doctest::Approx(170.f));
	const Vector2 right = editor::WorldToViewportImage(
		matrix, camera.position + Vector2(800.f, 0.f), image);
	CHECK(right.x == doctest::Approx(410.f));
	CHECK(right.y == doctest::Approx(170.f));
}
