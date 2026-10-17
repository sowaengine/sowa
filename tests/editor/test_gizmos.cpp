#include <doctest/doctest.h>

#include "ecs/components/transform2d.hpp"
#include "ecs/world.hpp"
#include "editor/camera.hpp"
#include "editor/gizmos.hpp"
#include "editor/scene_commands.hpp"
#include "editor/viewport.hpp"
#include "physics/physics_world.hpp"
#include "scene/scene.hpp"
#include "script/script_server.hpp"

using namespace sowa;

TEST_CASE("VisibleWorldBounds follows editor pan and zoom") {
	editor::EditorCamera camera;
	camera.position = Vector2(100.f, -50.f);
	camera.zoom = 2.f;
	const Rect visible = editor::VisibleWorldBounds(
		editor::ViewProjection(camera, Vector2(800.f, 600.f)));

	CHECK(visible.x == doctest::Approx(-700.f));
	CHECK(visible.y == doctest::Approx(-650.f));
	CHECK(visible.w == doctest::Approx(1600.f));
	CHECK(visible.h == doctest::Approx(1200.f));
}

TEST_CASE("AdaptiveGridLines stay on world multiples across negative coordinates") {
	const editor::AdaptiveGrid grid = editor::AdaptiveGridLines(
		Rect(-130.f, -70.f, 260.f, 140.f), Vector2(260.f, 140.f));
	REQUIRE(grid.minorSpacing == doctest::Approx(32.f));
	REQUIRE(grid.majorSpacing == doctest::Approx(128.f));
	REQUIRE(grid.lines.size() == 12);

	CHECK(grid.lines[0].line.a.x == doctest::Approx(-128.f));
	CHECK(grid.lines[0].kind == editor::GridLineKind::Major);
	CHECK(grid.lines[1].line.a.x == doctest::Approx(-96.f));
	CHECK(grid.lines[1].kind == editor::GridLineKind::Minor);
	CHECK(grid.lines[7].line.a.x == doctest::Approx(128.f));
	CHECK(grid.lines[7].kind == editor::GridLineKind::Major);
	CHECK(grid.lines[8].line.a.y == doctest::Approx(-64.f));
	CHECK(grid.lines[11].line.a.y == doctest::Approx(64.f));
	for (const editor::AdaptiveGridLine &line : grid.lines) {
		CHECK_FALSE((line.line.a.x == 0.f && line.line.b.x == 0.f));
		CHECK_FALSE((line.line.a.y == 0.f && line.line.b.y == 0.f));
	}
}

TEST_CASE("AdaptiveGridLines increase spacing as the camera zooms out") {
	const editor::AdaptiveGrid near = editor::AdaptiveGridLines(
		Rect(-400.f, -300.f, 800.f, 600.f), Vector2(800.f, 600.f));
	const editor::AdaptiveGrid far = editor::AdaptiveGridLines(
		Rect(-3200.f, -2400.f, 6400.f, 4800.f), Vector2(800.f, 600.f));

	REQUIRE(near.minorSpacing == doctest::Approx(32.f));
	CHECK(far.minorSpacing == doctest::Approx(256.f));
	CHECK(far.majorSpacing == doctest::Approx(1024.f));
	// Eight times the world spacing at eight times the world view keeps the
	// subdivision density identical on screen.
	CHECK(far.lines.size() == near.lines.size());
}

TEST_CASE("AdaptiveGridLines coarsen instead of disappearing over budget") {
	const editor::AdaptiveGrid grid = editor::AdaptiveGridLines(
		Rect(-10000.f, -10000.f, 20000.f, 20000.f),
		Vector2(20000.f, 20000.f), editor::kTargetMinorGridPixels,
		editor::kGizmoGridSubdivisions, 100);

	CHECK_FALSE(grid.lines.empty());
	CHECK(grid.lines.size() <= 100);
	CHECK(grid.minorSpacing > 32.f);
}

TEST_CASE("AdaptiveGridLines reject invalid geometry") {
	CHECK(editor::AdaptiveGridLines(Rect(0.f, 0.f, 0.f, 10.f),
									Vector2(100.f, 100.f))
			  .lines.empty());
	CHECK(editor::AdaptiveGridLines(Rect(0.f, 0.f, 10.f, 10.f),
									Vector2(0.f, 100.f))
			  .lines.empty());
}

TEST_CASE("VisibleOriginAxes span the visible rectangle") {
	const auto axes = editor::VisibleOriginAxes(Rect(-100.f, -50.f, 300.f, 200.f));
	REQUIRE(axes.x);
	REQUIRE(axes.y);
	CHECK(axes.x->a == Vector2(-100.f, 0.f));
	CHECK(axes.x->b == Vector2(200.f, 0.f));
	CHECK(axes.y->a == Vector2(0.f, -50.f));
	CHECK(axes.y->b == Vector2(0.f, 150.f));
}

TEST_CASE("VisibleOriginAxes omit axes outside the view") {
	const auto axes = editor::VisibleOriginAxes(Rect(10.f, 20.f, 100.f, 100.f));
	CHECK_FALSE(axes.x);
	CHECK_FALSE(axes.y);
}

TEST_CASE("SelectionWorldPosition requires Transform2D and includes parents") {
	World world;
	Entity plain = world.CreateEntity();
	CHECK_FALSE(editor::SelectionWorldPosition(plain));

	Entity parent = world.CreateEntity();
	Transform2D parentTransform;
	parentTransform.position = Vector2(100.f, 50.f);
	parentTransform.rotation = 90.f;
	parent.Add(parentTransform);

	Entity child = world.CreateEntity();
	Transform2D childTransform;
	childTransform.position = Vector2(10.f, 0.f);
	child.Add(childTransform);
	REQUIRE(child.SetParent(parent));

	const auto position = editor::SelectionWorldPosition(child);
	REQUIRE(position);
	CHECK(position->x == doctest::Approx(100.f).epsilon(0.001));
	CHECK(position->y == doctest::Approx(60.f).epsilon(0.001));
}

TEST_CASE("IsInSceneTree crosses a prefab instance's nested SceneRoot") {
	World world;
	Scene host = CreateScene(world, "Host");
	Scene other = CreateScene(world, "Other");
	Entity instance = host.Root().CreateChild("Instance");
	instance.Add(SceneRoot{});
	instance.Add(SceneInstance{});
	Entity child = instance.CreateChild("Child");

	REQUIRE(SceneOf(child).Root() == instance);
	CHECK(editor::IsInSceneTree(instance, host.Root()));
	CHECK(editor::IsInSceneTree(child, host.Root()));
	CHECK_FALSE(editor::IsInSceneTree(child, other.Root()));
}

namespace {
struct TransformGizmoFixture {
	World world;
	Entity entity = world.CreateEntity();
	Rect image{50.f, 30.f, 400.f, 300.f};
	glm::mat4 vp = editor::ViewProjection({}, {800.f, 600.f});

	TransformGizmoFixture() {
		entity.Add<Transform2D>();
	}

	editor::GizmoDrag begin(editor::GizmoMode mode, editor::GizmoHandle handle, Vector2 mouse) {
		const auto drag = editor::BeginGizmoDrag(entity, mode, handle, vp, image, mouse);
		REQUIRE(drag);
		return *drag;
	}
};

using Mode = editor::GizmoMode;
using Handle = editor::GizmoHandle;
} // namespace

TEST_CASE("Transform gizmo handles have stable screen size and distinct hit targets") {
	TransformGizmoFixture f;
	const auto geometry = editor::BuildGizmo(f.entity, Mode::Position, f.vp, f.image);
	REQUIRE(geometry);
	CHECK(geometry->origin == Vector2(250.f, 180.f));
	CHECK(editor::HitTestGizmo(*geometry, Mode::Position, {250.f, 180.f}) == Handle::Center);
	CHECK(editor::HitTestGizmo(*geometry, Mode::Position, {315.f, 180.f}) == Handle::X);
	CHECK(editor::HitTestGizmo(*geometry, Mode::Scale, {250.f, 108.f}) == Handle::Y);
	CHECK(editor::HitTestGizmo(*geometry, Mode::Position, {280.f, 150.f}) == Handle::None);
	CHECK(editor::HitTestGizmo(*geometry, Mode::Rotation, {306.f, 180.f}) == Handle::Ring);
	CHECK(editor::HitTestGizmo(*geometry, Mode::Rotation, geometry->origin) == Handle::None);

	editor::EditorCamera camera;
	camera.zoom = 20.f;
	const auto zoomed = editor::BuildGizmo(f.entity, Mode::Position,
		editor::ViewProjection(camera, {800.f, 600.f}), f.image);
	REQUIRE(zoomed);
	CHECK(zoomed->xDirection == geometry->xDirection);
	CHECK(zoomed->yDirection == geometry->yDirection);
	CHECK(editor::HitTestGizmo(*zoomed, Mode::Position, {315.f, 180.f}) == Handle::X);
}

TEST_CASE("Position gizmo converts letterboxed pixels to world motion and constrains axes") {
	TransformGizmoFixture f;
	auto drag = f.begin(Mode::Position, Handle::Center, {250.f, 180.f});
	auto transform = editor::UpdateGizmoDrag(drag, {265.f, 170.f});
	CHECK(transform.position.x == doctest::Approx(30.f));
	CHECK(transform.position.y == doctest::Approx(20.f));
	// Repeated updates are based on mouse-down, not accumulated frame deltas.
	CHECK(editor::UpdateGizmoDrag(drag, {265.f, 170.f}).position == transform.position);

	drag = f.begin(Mode::Position, Handle::X, {310.f, 180.f});
	transform = editor::UpdateGizmoDrag(drag, {325.f, 150.f});
	CHECK(transform.position.x == doctest::Approx(30.f));
	CHECK(transform.position.y == doctest::Approx(0.f));
	drag = f.begin(Mode::Position, Handle::Y, {250.f, 120.f});
	transform = editor::UpdateGizmoDrag(drag, {275.f, 110.f});
	CHECK(transform.position.x == doctest::Approx(0.f));
	CHECK(transform.position.y == doctest::Approx(20.f));
}

TEST_CASE("Position gizmo respects pan zoom and transformed ancestors through grouping entities") {
	TransformGizmoFixture f;
	Entity parent = f.world.CreateEntity();
	Transform2D parentTransform;
	parentTransform.position = {80.f, 40.f};
	parentTransform.rotation = 90.f;
	parentTransform.scale = {2.f, 3.f};
	parent.Add(parentTransform);
	Entity group = parent.CreateChild();
	REQUIRE(f.entity.SetParent(group));
	editor::EditorCamera camera;
	camera.position = {100.f, -50.f};
	camera.zoom = 2.f;
	f.vp = editor::ViewProjection(camera, {800.f, 600.f});
	auto drag = f.begin(Mode::Position, Handle::X, {300.f, 200.f});
	const auto transform = editor::UpdateGizmoDrag(drag, {315.f, 190.f});
	CHECK(transform.position.x == doctest::Approx(0.f).epsilon(0.001));
	CHECK(transform.position.y == doctest::Approx(-20.f));
	*f.entity.Get<Transform2D>() = transform;
	const auto position = editor::SelectionWorldPosition(f.entity);
	REQUIRE(position);
	CHECK(position->x == doctest::Approx(140.f));
	CHECK(position->y == doctest::Approx(40.f));
}

TEST_CASE("Rotation gizmo rotates counterclockwise and crosses angle wrap continuously") {
	TransformGizmoFixture f;
	auto drag = f.begin(Mode::Rotation, Handle::Ring, {306.f, 180.f});
	CHECK(editor::UpdateGizmoDrag(drag, {250.f, 124.f}).rotation == doctest::Approx(90.f));
	CHECK(editor::UpdateGizmoDrag(drag, {194.f, 180.f}).rotation == doctest::Approx(180.f));
	CHECK(editor::UpdateGizmoDrag(drag, {250.f, 236.f}).rotation == doctest::Approx(270.f));
	CHECK(editor::UpdateGizmoDrag(drag, {306.f, 180.f}).rotation == doctest::Approx(360.f));
	// Crossing the pivot itself must not introduce an arbitrary atan2(0,0) angle.
	CHECK(editor::UpdateGizmoDrag(drag, {250.f, 180.f}).rotation == doctest::Approx(360.f));
}

TEST_CASE("Rotation gizmo maps through reflected nonuniform parents") {
	TransformGizmoFixture f;
	Entity parent = f.world.CreateEntity();
	Transform2D transform;
	transform.scale = {-2.f, 3.f};
	parent.Add(transform);
	REQUIRE(f.entity.SetParent(parent));
	auto drag = f.begin(Mode::Rotation, Handle::Ring, {194.f, 180.f});
	const auto rotated = editor::UpdateGizmoDrag(drag, {250.f, 124.f});
	CHECK(rotated.rotation == doctest::Approx(90.f));
	CHECK(rotated.position == Vector2(0.f));
	CHECK(rotated.scale == Vector2(1.f));
}

TEST_CASE("Scale gizmo follows rotated local axes and changes one scale component") {
	TransformGizmoFixture f;
	f.entity.Get<Transform2D>()->rotation = 90.f;
	f.entity.Get<Transform2D>()->scale = {2.f, 3.f};
	const auto geometry = editor::BuildGizmo(f.entity, Mode::Scale, f.vp, f.image);
	REQUIRE(geometry);
	CHECK(geometry->xDirection.x == doctest::Approx(0.f).epsilon(0.001));
	CHECK(geometry->xDirection.y == doctest::Approx(-1.f));
	auto drag = f.begin(Mode::Scale, Handle::X, {250.f, 108.f});
	const auto scaled = editor::UpdateGizmoDrag(drag, {250.f, 36.f});
	CHECK(scaled.scale.x == doctest::Approx(4.f));
	CHECK(scaled.scale.y == doctest::Approx(3.f));
	CHECK(scaled.rotation == doctest::Approx(90.f));
	CHECK(scaled.position == Vector2(0.f));
}

TEST_CASE("Scale gizmo can restore zero scale and uniformly scale mirrored entities") {
	TransformGizmoFixture f;
	f.entity.Get<Transform2D>()->scale = {0.f, 1.f};
	auto drag = f.begin(Mode::Scale, Handle::X, {322.f, 180.f});
	CHECK(editor::UpdateGizmoDrag(drag, {394.f, 180.f}).scale.x == doctest::Approx(1.f));
	f.entity.Get<Transform2D>()->scale = {-2.f, 3.f};
	drag = f.begin(Mode::Scale, Handle::Center, {250.f, 180.f});
	const auto scaled = editor::UpdateGizmoDrag(drag, {286.f, 144.f});
	CHECK(scaled.scale.x == doctest::Approx(-2.f * std::exp(1.f)));
	CHECK(scaled.scale.y == doctest::Approx(3.f * std::exp(1.f)));
}

TEST_CASE("Transform gizmo rejects missing transforms and singular parents") {
	TransformGizmoFixture f;
	CHECK_FALSE(editor::BuildGizmo({}, Mode::Position, f.vp, f.image));
	Entity plain = f.world.CreateEntity();
	CHECK_FALSE(editor::BuildGizmo(plain, Mode::Position, f.vp, f.image));
	Entity parent = f.world.CreateEntity();
	parent.Add<Transform2D>();
	parent.Get<Transform2D>()->scale.x = 0.f;
	REQUIRE(f.entity.SetParent(parent));
	CHECK_FALSE(editor::BeginGizmoDrag(f.entity, Mode::Position, Handle::X, f.vp, f.image, {}));
}

TEST_CASE("Completed transform drags record one undoable scene field edit") {
	for (Mode mode : {Mode::Position, Mode::Rotation, Mode::Scale}) {
		TransformGizmoFixture f;
		Scene scene = CreateScene(f.world, "GizmoScene");
		scene.SetSourcePath("tmp://gizmo.scene");
		REQUIRE(f.entity.SetParent(scene.Root()));
		std::vector<editor::SceneDocumentSaveState> states;
		editor::UpdateSceneDocumentStates(states, {scene});
		auto *state = editor::FindSceneDocumentState(states, scene);
		REQUIRE(state);
		state->history.MarkSaved();
		const auto type = f.world.FindComponent("Transform2D");
		const auto *field = type.FindField(mode == Mode::Position ? "position"
				: mode == Mode::Rotation						  ? "rotation"
																  : "scale");
		REQUIRE(field);
		editor::FieldValue before, after;
		REQUIRE(editor::CaptureFieldValue(f.entity, type, *field, before));
		auto drag =
			f.begin(mode, mode == Mode::Rotation ? Handle::Ring : Handle::Center, {306.f, 180.f});
		for (Vector2 mouse :
			{Vector2(280.f, 160.f), Vector2(270.f, 140.f), Vector2(250.f, 100.f)}) {
			*f.entity.Get<Transform2D>() = editor::UpdateGizmoDrag(drag, mouse);
		}
		REQUIRE(editor::CaptureFieldValue(f.entity, type, *field, after));
		REQUIRE(editor::RecordFieldEdit(states, scene, f.entity, type, *field, before, after));
		CHECK(editor::IsSceneDocumentUnsaved(states, scene));
		REQUIRE(editor::UndoSceneDocument(states, scene));
		editor::FieldValue restored;
		REQUIRE(editor::CaptureFieldValue(f.entity, type, *field, restored));
		CHECK(restored == before);
		CHECK_FALSE(editor::IsSceneDocumentUnsaved(states, scene));
		CHECK_FALSE(editor::UndoSceneDocument(states, scene));
		REQUIRE(editor::RedoSceneDocument(states, scene));
		REQUIRE(editor::CaptureFieldValue(f.entity, type, *field, restored));
		CHECK(restored == after);
		CHECK_FALSE(editor::RecordFieldEdit(states, scene, f.entity, type, *field, after, after));
	}
}

TEST_CASE("Hidden gizmos have no handles and cannot start a drag") {
	TransformGizmoFixture f;
	const auto gizmo = editor::BuildGizmo(f.entity, Mode::Position, f.vp, f.image);
	REQUIRE(gizmo);
	CHECK_FALSE(editor::BuildGizmo(f.entity, Mode::None, f.vp, f.image));
	CHECK(editor::HitTestGizmo(*gizmo, Mode::None, gizmo->origin) == Handle::None);
	CHECK_FALSE(
		editor::BeginGizmoDrag(f.entity, Mode::None, Handle::Center, f.vp, f.image, gizmo->origin));
}

TEST_CASE("ray endpoint gizmo maps inherited transforms and supports field undo") {
	World world;
	PhysicsWorld physics;
	physics.Init(world);
	Scene scene = CreateScene(world, "Level");
	Entity parent = scene.CreateEntity("Parent");
	parent.Add(Transform2D{{100.f, -40.f}, 90.f, {-2.f, 3.f}});
	Entity ray = world.CreateEntity();
	ray.Add(Transform2D{{10.f, 0.f}});
	ray.Add(Raycast2D{});
	REQUIRE(ray.SetParent(parent));
	const auto line = editor::RaycastGizmoLine(ray);
	REQUIRE(line);
	CHECK(line->a.x == doctest::Approx(100.f));
	CHECK(line->a.y == doctest::Approx(-60.f));
	CHECK(line->b.x == doctest::Approx(250.f));
	CHECK(line->b.y == doctest::Approx(-60.f));
	editor::EditorCamera camera;
	const auto projection = editor::ViewProjection(camera, {800.f, 600.f});
	const Rect image{30.f, 50.f, 400.f, 300.f};
	const auto screen = editor::WorldToViewportImage(projection, line->b, image);
	const auto local = editor::RaycastTargetFromViewport(ray, projection, image, screen);
	REQUIRE(local);
	CHECK(local->x == doctest::Approx(0.f).epsilon(.001));
	CHECK(local->y == doctest::Approx(-50.f));
	parent.Get<Transform2D>()->scale.x = 0;
	CHECK_FALSE(editor::RaycastTargetFromViewport(ray, projection, image, screen));
	CHECK_FALSE(editor::RaycastGizmoLine({}));
}

TEST_CASE("ray and mask authoring support undo redo and transform dependencies") {
	World world;
	PhysicsWorld physics;
	physics.Init(world);
	ScriptServer scripts(world);
	Scene scene = CreateScene(world, "RayScene");
	scene.SetSourcePath("tmp://ray.scene");
	Entity ray = scene.CreateEntity("Ray");
	std::vector<editor::SceneDocumentSaveState> states;
	editor::UpdateSceneDocumentStates(states, {scene});
	const auto type = world.FindComponent("Raycast2D");
	REQUIRE(editor::ExecuteAddComponent(states, scene, ray, type, scripts));
	CHECK(ray.Has<Transform2D>());
	REQUIRE(editor::UndoSceneDocument(states, scene));
	CHECK_FALSE(ray.Has<Transform2D>());
	REQUIRE(editor::RedoSceneDocument(states, scene));
	for (const std::string name : {"targetPosition", "collisionMask"}) {
		const auto *field = type.FindField(name);
		REQUIRE(field);
		editor::FieldValue before, after, restored;
		REQUIRE(editor::CaptureFieldValue(ray, type, *field, before));
		if (name == "targetPosition") {
			ray.Get<Raycast2D>()->targetPosition = {20.f, -80.f};
		} else {
			ray.Get<Raycast2D>()->collisionMask = 0x80000000u;
		}
		REQUIRE(editor::CaptureFieldValue(ray, type, *field, after));
		REQUIRE(editor::RecordFieldEdit(states, scene, ray, type, *field, before, after));
		REQUIRE(editor::UndoSceneDocument(states, scene));
		REQUIRE(editor::CaptureFieldValue(ray, type, *field, restored));
		CHECK(restored == before);
		REQUIRE(editor::RedoSceneDocument(states, scene));
		REQUIRE(editor::CaptureFieldValue(ray, type, *field, restored));
		CHECK(restored == after);
	}
}
