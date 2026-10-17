#include <vector>

#include <doctest/doctest.h>

#include <glm/glm.hpp>

#include "ecs/components/camera2d.hpp"
#include "ecs/components/transform2d.hpp"
#include "ecs/entity.hpp"
#include "ecs/world.hpp"
#include "math/vector2.hpp"

using namespace sowa;

namespace {

constexpr f32 kEpsilon = 0.001f;
const Vector2 kViewport(800.f, 600.f);

// Where a world-space point lands in normalised device coordinates. NDC is
// what the projection actually feeds the GPU, so this checks the matrix the
// way the renderer consumes it rather than re-deriving its internals.
Vector2 ToNdc(const glm::mat4 &viewProjection, Vector2 worldPoint) {
	const glm::vec4 clip = viewProjection * glm::vec4(worldPoint.x, worldPoint.y, 0.f, 1.f);
	return Vector2(clip.x, clip.y);
}

void CheckNdc(Vector2 actual, Vector2 expected) {
	CHECK(actual.x == doctest::Approx(expected.x).epsilon(kEpsilon));
	CHECK(actual.y == doctest::Approx(expected.y).epsilon(kEpsilon));
}

Entity MakeCamera(World &world, Vector2 position) {
	Entity camera = world.CreateEntity();
	Transform2D transform;
	transform.position = position;
	camera.Add(transform);
	camera.Add(Camera2D{});
	return camera;
}

} // namespace

TEST_CASE("Camera2D is a built-in with its authored fields described") {
	World world;

	const ComponentType type = world.FindComponent("Camera2D");
	REQUIRE(type.IsValid());
	CHECK(type.FindField("offset") != nullptr);
	CHECK(type.FindField("rotatable") != nullptr);
	CHECK(world.RegisterComponent<Camera2D>("Camera2D").Error() == EcsError::DuplicateName);
}

TEST_CASE("With no camera the projection puts the origin bottom-left") {
	const glm::mat4 matrix = CalculateBlankViewProjection(kViewport);

	// Matches how the renderer framed the world before cameras existed.
	CheckNdc(ToNdc(matrix, Vector2(0.f, 0.f)), Vector2(-1.f, -1.f));
	CheckNdc(ToNdc(matrix, Vector2(800.f, 600.f)), Vector2(1.f, 1.f));
	CheckNdc(ToNdc(matrix, Vector2(400.f, 300.f)), Vector2(0.f, 0.f));
}

TEST_CASE("A camera centres the view on its own position") {
	World world;
	Entity camera = MakeCamera(world, Vector2(1000.f, 2000.f));

	const glm::mat4 matrix = CalculateViewProjection(camera, kViewport);

	CheckNdc(ToNdc(matrix, Vector2(1000.f, 2000.f)), Vector2(0.f, 0.f));
	CheckNdc(ToNdc(matrix, Vector2(600.f, 1700.f)), Vector2(-1.f, -1.f));
	CheckNdc(ToNdc(matrix, Vector2(1400.f, 2300.f)), Vector2(1.f, 1.f));
}

TEST_CASE("Moving the camera moves the view") {
	World world;
	Entity camera = MakeCamera(world, Vector2(0.f, 0.f));

	REQUIRE(ToNdc(CalculateViewProjection(camera, kViewport), Vector2(0.f, 0.f)).x ==
			doctest::Approx(0.f).epsilon(kEpsilon));

	camera.Get<Transform2D>()->position = Vector2(400.f, 0.f);

	// The world point the camera moved off is now to the left of centre.
	CheckNdc(ToNdc(CalculateViewProjection(camera, kViewport), Vector2(0.f, 0.f)),
			 Vector2(-1.f, 0.f));
}

TEST_CASE("Camera scale zooms out, because the view is the inverse transform") {
	World world;
	Entity camera = MakeCamera(world, Vector2(0.f, 0.f));

	camera.Get<Transform2D>()->scale = Vector2(2.f, 2.f);
	const glm::mat4 matrix = CalculateViewProjection(camera, kViewport);

	// Twice the scale shows twice the world, so the old edge is now halfway
	// out. This is the zoom control - there is no separate zoom field.
	CheckNdc(ToNdc(matrix, Vector2(400.f, 0.f)), Vector2(0.5f, 0.f));
	CheckNdc(ToNdc(matrix, Vector2(800.f, 0.f)), Vector2(1.f, 0.f));
}

TEST_CASE("Camera rotation is ignored unless rotatable is set") {
	World world;
	Entity camera = MakeCamera(world, Vector2(0.f, 0.f));
	camera.Get<Transform2D>()->rotation = 90.f;

	// Default off: a camera parented to a spinning player must not spin the
	// screen, so a point out along +X stays out along +X.
	CheckNdc(ToNdc(CalculateViewProjection(camera, kViewport), Vector2(400.f, 0.f)),
			 Vector2(1.f, 0.f));

	camera.Get<Camera2D>()->rotatable = true;

	// Rotating the camera +90 degrees swings a world +X point to screen -Y.
	const Vector2 rotated = ToNdc(CalculateViewProjection(camera, kViewport), Vector2(400.f, 0.f));
	CHECK(rotated.x == doctest::Approx(0.f).epsilon(kEpsilon));
	CHECK(rotated.y == doctest::Approx(-400.f / 300.f).epsilon(kEpsilon));
}

TEST_CASE("The camera offset shifts the view without moving the entity") {
	World world;
	Entity camera = MakeCamera(world, Vector2(0.f, 0.f));
	camera.Get<Camera2D>()->offset = Vector2(400.f, 0.f);

	// Same effect on the view as moving the camera, but the transform is
	// untouched - which is what makes it usable for shake or look-ahead.
	CheckNdc(ToNdc(CalculateViewProjection(camera, kViewport), Vector2(0.f, 0.f)),
			 Vector2(-1.f, 0.f));
	CHECK(camera.Get<Transform2D>()->position.x == doctest::Approx(0.f));
}

TEST_CASE("A camera inherits its parent's transform, which is how following works") {
	World world;

	Entity player = world.CreateEntity();
	Transform2D playerTransform;
	playerTransform.position = Vector2(500.f, 100.f);
	player.Add(playerTransform);

	Entity camera = MakeCamera(world, Vector2(0.f, 0.f));
	REQUIRE(camera.SetParent(player));

	// No follow code anywhere - the camera is just a child.
	CheckNdc(ToNdc(CalculateViewProjection(camera, kViewport), Vector2(500.f, 100.f)),
			 Vector2(0.f, 0.f));

	player.Get<Transform2D>()->position = Vector2(900.f, 100.f);
	CheckNdc(ToNdc(CalculateViewProjection(camera, kViewport), Vector2(900.f, 100.f)),
			 Vector2(0.f, 0.f));
}

TEST_CASE("The view is correct before the first Progress") {
	World world;
	Entity camera = MakeCamera(world, Vector2(300.f, 0.f));

	// No pass has run, so the cached world matrix is still identity - the
	// camera walks the hierarchy itself rather than trusting it.
	REQUIRE(camera.Get<Transform2D>()->worldMatrix == glm::mat4(1.f));
	CheckNdc(ToNdc(CalculateViewProjection(camera, kViewport), Vector2(300.f, 0.f)),
			 Vector2(0.f, 0.f));
}

// Camera anchor.

TEST_CASE("The default anchor is at the viewport center") {
	World world;
	Entity camera = MakeCamera(world, Vector2(500.f, 400.f));

	CHECK(camera.Get<Camera2D>()->anchor.x == doctest::Approx(0.5f));
	CheckNdc(ToNdc(CalculateViewProjection(camera, kViewport), Vector2(500.f, 400.f)),
			 Vector2(0.f, 0.f));
}

TEST_CASE("Anchor 0 puts the camera at the bottom-left, 1 at the top-right") {
	World world;
	Entity camera = MakeCamera(world, Vector2(0.f, 0.f));

	camera.Get<Camera2D>()->anchor = Vector2(0.f, 0.f);
	CheckNdc(ToNdc(CalculateViewProjection(camera, kViewport), Vector2(0.f, 0.f)),
			 Vector2(-1.f, -1.f));

	camera.Get<Camera2D>()->anchor = Vector2(1.f, 1.f);
	CheckNdc(ToNdc(CalculateViewProjection(camera, kViewport), Vector2(0.f, 0.f)),
			 Vector2(1.f, 1.f));
}

TEST_CASE("An anchor can differ per axis") {
	World world;
	Entity camera = MakeCamera(world, Vector2(0.f, 0.f));

	// A vertical shooter wanting the player low and centred horizontally.
	camera.Get<Camera2D>()->anchor = Vector2(0.5f, 0.25f);

	const Vector2 ndc = ToNdc(CalculateViewProjection(camera, kViewport), Vector2(0.f, 0.f));
	CHECK(ndc.x == doctest::Approx(0.f).epsilon(kEpsilon));
	CHECK(ndc.y == doctest::Approx(-0.5f).epsilon(kEpsilon)); // a quarter up the screen
}

TEST_CASE("Anchoring shifts what the viewport shows, not just the camera dot") {
	World world;
	Entity camera = MakeCamera(world, Vector2(0.f, 0.f));
	// Side-scroller framing: camera on the left third, so most of the visible
	// world is ahead of it.
	camera.Get<Camera2D>()->anchor = Vector2(0.25f, 0.5f);

	const glm::mat4 matrix = CalculateViewProjection(camera, kViewport);

	// Visible span is [-200, +600] rather than [-400, +400].
	CheckNdc(ToNdc(matrix, Vector2(-200.f, 0.f)), Vector2(-1.f, 0.f));
	CheckNdc(ToNdc(matrix, Vector2(600.f, 0.f)), Vector2(1.f, 0.f));
}

TEST_CASE("The anchor is independent of zoom") {
	World world;
	Entity camera = MakeCamera(world, Vector2(0.f, 0.f));
	camera.Get<Camera2D>()->anchor = Vector2(0.25f, 0.75f);

	const Vector2 before = ToNdc(CalculateViewProjection(camera, kViewport), Vector2(0.f, 0.f));

	camera.Get<Transform2D>()->scale = Vector2(3.f, 3.f);
	const Vector2 after = ToNdc(CalculateViewProjection(camera, kViewport), Vector2(0.f, 0.f));

	// Zooming changes how much world fits, not where on screen the camera's
	// own position sits - the anchor is a screen-space concept.
	CheckNdc(after, before);
	CHECK(before.x == doctest::Approx(-0.5f).epsilon(kEpsilon));
	CHECK(before.y == doctest::Approx(0.5f).epsilon(kEpsilon));
}

TEST_CASE("The anchor composes with the camera offset") {
	World world;
	Entity camera = MakeCamera(world, Vector2(0.f, 0.f));
	camera.Get<Camera2D>()->anchor = Vector2(0.f, 0.5f);
	camera.Get<Camera2D>()->offset = Vector2(100.f, 0.f);

	// Offset moves the viewpoint in world space, anchor decides where that
	// viewpoint lands on screen; the two stack rather than fighting.
	CheckNdc(ToNdc(CalculateViewProjection(camera, kViewport), Vector2(100.f, 0.f)),
			 Vector2(-1.f, 0.f));
}

TEST_CASE("The anchor is a described field so it survives serialization") {
	World world;
	const ComponentType type = world.FindComponent("Camera2D");
	REQUIRE(type.IsValid());
	CHECK(type.FindField("anchor") != nullptr);
}

TEST_CASE("A camera with no transform sits at the origin") {
	World world;
	Entity camera = world.CreateEntity();
	camera.Add(Camera2D{});

	CheckNdc(ToNdc(CalculateViewProjection(camera, kViewport), Vector2(0.f, 0.f)),
			 Vector2(0.f, 0.f));
}

TEST_CASE("CalculateViewProjection on a dead camera gives the blank projection") {
	World world;
	Entity dead = MakeCamera(world, Vector2(500.f, 500.f));
	dead.Destroy();

	const glm::mat4 matrix = CalculateViewProjection(dead, kViewport);
	CheckNdc(ToNdc(matrix, Vector2(0.f, 0.f)), Vector2(-1.f, -1.f));

	const glm::mat4 nullMatrix = CalculateViewProjection(Entity(), kViewport);
	CheckNdc(ToNdc(nullMatrix, Vector2(0.f, 0.f)), Vector2(-1.f, -1.f));
}
