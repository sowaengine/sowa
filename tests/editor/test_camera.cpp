#include <doctest/doctest.h>

#include <glm/glm.hpp>

#include "ecs/components/camera2d.hpp"
#include "ecs/components/transform2d.hpp"
#include "ecs/entity.hpp"
#include "ecs/world.hpp"
#include "editor/camera.hpp"
#include "math/vector2.hpp"

using namespace sowa;

namespace {

constexpr f32 kEpsilon = 0.001f;
const Vector2 kViewport(800.f, 600.f);

Vector2 ToNdc(const glm::mat4 &viewProjection, Vector2 worldPoint) {
	const glm::vec4 clip =
		viewProjection * glm::vec4(worldPoint.x, worldPoint.y, 0.f, 1.f);
	return Vector2(clip.x, clip.y);
}

void CheckNdc(Vector2 actual, Vector2 expected) {
	CHECK(actual.x == doctest::Approx(expected.x).epsilon(kEpsilon));
	CHECK(actual.y == doctest::Approx(expected.y).epsilon(kEpsilon));
}

} // namespace

TEST_CASE("PanDelta grabs the canvas: drag right moves the camera left") {
	const Vector2 delta =
		editor::PanDelta(Vector2(10.f, 20.f), 1.f, Vector2(800.f, 600.f),
						 Vector2(800.f, 600.f));
	CHECK(delta.x == doctest::Approx(-10.f));
	CHECK(delta.y == doctest::Approx(20.f));
}

TEST_CASE("PanDelta scales with zoom") {
	const Vector2 delta =
		editor::PanDelta(Vector2(10.f, 0.f), 2.f, Vector2(800.f, 600.f),
						 Vector2(800.f, 600.f));
	CHECK(delta.x == doctest::Approx(-20.f));
	CHECK(delta.y == doctest::Approx(0.f));
}

TEST_CASE("PanDelta maps through a letterboxed image") {
	// Game 800x600 drawn at half size: 10 window pixels are 20 game pixels.
	const Vector2 delta =
		editor::PanDelta(Vector2(10.f, 0.f), 1.f, Vector2(800.f, 600.f),
						 Vector2(400.f, 300.f));
	CHECK(delta.x == doctest::Approx(-20.f));
	CHECK(delta.y == doctest::Approx(0.f));
}

TEST_CASE("PanDelta is zero when the image has no size") {
	const Vector2 delta =
		editor::PanDelta(Vector2(10.f, 10.f), 1.f, Vector2(800.f, 600.f),
						 Vector2(0.f, 0.f));
	CHECK(delta.x == doctest::Approx(0.f));
	CHECK(delta.y == doctest::Approx(0.f));
}

TEST_CASE("ClampScrollDelta keeps a mouse notch and cuts a trackpad flick") {
	CHECK(editor::ClampScrollDelta(1.f) == doctest::Approx(1.f));
	CHECK(editor::ClampScrollDelta(-1.f) == doctest::Approx(-1.f));
	CHECK(editor::ClampScrollDelta(0.4f) == doctest::Approx(0.4f));
	CHECK(editor::ClampScrollDelta(8.f) == doctest::Approx(1.f));
	CHECK(editor::ClampScrollDelta(-5.f) == doctest::Approx(-1.f));
}

TEST_CASE("ZoomAfterScroll zooms in on scroll up") {
	CHECK(editor::ZoomAfterScroll(1.f, 1.f) == doctest::Approx(0.9f));
}

TEST_CASE("ZoomAfterScroll clamps to 0.1 and 100") {
	CHECK(editor::ZoomAfterScroll(0.1f, 10.f) == doctest::Approx(0.1f));
	CHECK(editor::ZoomAfterScroll(100.f, -10.f) == doctest::Approx(100.f));
}

TEST_CASE("ZoomCursorPan is zero when the cursor is at the image centre") {
	const Vector2 pan = editor::ZoomCursorPan(
		Vector2(400.f, 300.f), Vector2(800.f, 600.f), Vector2(800.f, 600.f),
		1.f, 0.9f);
	CHECK(pan.x == doctest::Approx(0.f));
	CHECK(pan.y == doctest::Approx(0.f));
}

TEST_CASE("ZoomCursorPan moves the camera toward a cursor right of centre") {
	// Right edge, vertical centre. Zoom in 1 → 0.9. The world point at +400
	// stays under the cursor only if the camera shifts +40.
	const Vector2 pan = editor::ZoomCursorPan(
		Vector2(800.f, 300.f), Vector2(800.f, 600.f), Vector2(800.f, 600.f),
		1.f, 0.9f);
	CHECK(pan.x == doctest::Approx(40.f));
	CHECK(pan.y == doctest::Approx(0.f));
}

TEST_CASE("ZoomCursorPan moves the camera toward a cursor at the top") {
	const Vector2 pan = editor::ZoomCursorPan(
		Vector2(400.f, 0.f), Vector2(800.f, 600.f), Vector2(800.f, 600.f),
		1.f, 0.9f);
	CHECK(pan.x == doctest::Approx(0.f));
	CHECK(pan.y == doctest::Approx(30.f));
}

TEST_CASE("ZoomCursorPan maps through a letterboxed image") {
	const Vector2 pan = editor::ZoomCursorPan(
		Vector2(400.f, 150.f), Vector2(400.f, 300.f), Vector2(800.f, 600.f),
		1.f, 0.9f);
	CHECK(pan.x == doctest::Approx(40.f));
	CHECK(pan.y == doctest::Approx(0.f));
}

TEST_CASE("ZoomCursorPan keeps the world point under the cursor in NDC") {
	editor::EditorCamera camera;
	const Vector2 gameSize = kViewport;
	const f32 oldZoom = 1.f;
	const f32 newZoom = 0.9f;
	const Vector2 pan = editor::ZoomCursorPan(
		Vector2(800.f, 300.f), gameSize, gameSize, oldZoom, newZoom);

	camera.zoom = oldZoom;
	const Vector2 worldUnderCursor(400.f, 0.f);
	const Vector2 ndcBefore =
		ToNdc(editor::ViewProjection(camera, gameSize), worldUnderCursor);

	camera.zoom = newZoom;
	camera.position += pan;
	const Vector2 ndcAfter =
		ToNdc(editor::ViewProjection(camera, gameSize), worldUnderCursor);
	CheckNdc(ndcAfter, ndcBefore);
}

TEST_CASE("Editor ViewProjection centres the camera position") {
	editor::EditorCamera camera;
	camera.position = Vector2(1000.f, 2000.f);
	const glm::mat4 matrix = editor::ViewProjection(camera, kViewport);
	CheckNdc(ToNdc(matrix, Vector2(1000.f, 2000.f)), Vector2(0.f, 0.f));
}

TEST_CASE("Editor ViewProjection zoom 2 shows twice the world") {
	editor::EditorCamera camera;
	camera.zoom = 2.f;
	const glm::mat4 matrix = editor::ViewProjection(camera, kViewport);
	CheckNdc(ToNdc(matrix, Vector2(400.f, 0.f)), Vector2(0.5f, 0.f));
	CheckNdc(ToNdc(matrix, Vector2(800.f, 0.f)), Vector2(1.f, 0.f));
}

TEST_CASE("Editor ViewProjection matches a Camera2D at the same transform") {
	World world;
	Entity entity = world.CreateEntity();
	Transform2D transform;
	transform.position = Vector2(100.f, 50.f);
	transform.scale = Vector2(2.f, 2.f);
	entity.Add(transform);
	entity.Add(Camera2D{});

	editor::EditorCamera camera;
	camera.position = Vector2(100.f, 50.f);
	camera.zoom = 2.f;

	const glm::mat4 editorMatrix = editor::ViewProjection(camera, kViewport);
	const glm::mat4 sceneMatrix = CalculateViewProjection(entity, kViewport);
	CheckNdc(ToNdc(editorMatrix, Vector2(100.f, 50.f)),
			 ToNdc(sceneMatrix, Vector2(100.f, 50.f)));
	CheckNdc(ToNdc(editorMatrix, Vector2(500.f, 50.f)),
			 ToNdc(sceneMatrix, Vector2(500.f, 50.f)));
}
