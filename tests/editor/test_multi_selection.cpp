#include <algorithm>
#include <doctest/doctest.h>
#include <glm/gtc/matrix_transform.hpp>
#include <limits>

#include "core/filesystem/file_system.hpp"
#include "ecs/components/editor_entity_id.hpp"
#include "ecs/world.hpp"
#include "editor/bulk_inspector.hpp"
#include "editor/group_transform.hpp"
#include "editor/resource_inspector.hpp"
#include "editor/viewport.hpp"
#include "math/matrix.hpp"
#include "resource/image_texture.hpp"
#include "scene/scene_manager.hpp"
#include "scene/scene_serializer.hpp"
#include "scene/scene_snapshot.hpp"

using namespace sowa;
using namespace sowa::editor;

namespace {
struct Links {
	Entity target;
	Vector2 point;
	Color color;
	bool enabled = false;
	ResourceID texture;
};

struct Fixture {
	World world;
	ResourceRegistry resources;
	Scene scene = CreateScene(world, "Scene");
	std::vector<SceneDocumentSaveState> states;
	Entity a = scene.CreateEntity("A"), b = scene.CreateEntity("B");

	Fixture() {
		world.RegisterComponent<Links>("Links")
			.Field("target", &Links::target)
			.Field("point", &Links::point)
			.Field("color", &Links::color)
			.Field("enabled", &Links::enabled)
			.Field("texture", &Links::texture);
		a.Add(Transform2D{});
		b.Add(Transform2D{});
		a.Add(Links{b, {1, 2}, {1, 0, 0, 1}, false, {}});
		b.Add(Links{a, {3, 4}, {0, 1, 0, 0.5f}, true, {}});
		UpdateSceneDocumentStates(states, {scene});
		state().context->selection.Assign({a, b}, a);
		state().history.MarkSaved();
	}

	~Fixture() {
		states.clear();
		UnloadScene(scene, resources);
	}

	SceneDocumentSaveState &state() {
		return states.front();
	}

	EntitySelection &selection() {
		return state().context->selection;
	}

	SharedInspectorField field(std::string_view name) {
		auto type = world.FindComponent("Links");
		return {"Links", *type.FindField(name)};
	}
};

void matrixNear(const glm::mat4 &a, const glm::mat4 &b) {
	for (int c = 0; c < 4; ++c) {
		for (int r = 0; r < 4; ++r) {
			CHECK(a[c][r] == doctest::Approx(b[c][r]).epsilon(0.0002));
		}
	}
}
} // namespace

TEST_CASE("B10 selection toggles, active fallback and visible ranges") {
	Fixture f;
	Entity c = f.scene.CreateEntity("C"), hidden = f.scene.CreateEntity("Hidden");
	EntitySelection selected;
	selected.Set(f.a);
	selected.Click(f.b, SelectionMode::Add);
	CHECK(selected.Active() == f.b);
	selected.Click(f.b, SelectionMode::Toggle);
	CHECK(selected.Active() == f.a);
	selected.Range(c, {f.a, f.b, c}, false);
	CHECK(selected.Members() == std::vector<Entity>{f.a, f.b, c});
	CHECK(selected.Anchor() == f.a);
	selected.Range(f.b, {f.a, f.b, c}, false);
	CHECK(selected.Members() == std::vector<Entity>{f.a, f.b});
	selected.Click(hidden, SelectionMode::Add);
	selected.Range(c, {f.a, f.b, c}, false);
	CHECK(selected.Members() == std::vector<Entity>{c});
	selected.Assign({f.a, f.a, f.b}, f.a);
	CHECK(selected.Members().size() == 2);
	selected.Click(f.a, SelectionMode::Toggle);
	CHECK(selected.Active() == f.b);
	selected.Click(f.b, SelectionMode::Toggle);
	CHECK_FALSE(selected.Active().IsAlive());
	CHECK_FALSE(selected.Anchor().IsAlive());
}

TEST_CASE("B10 selection assignment accepts its own membership") {
	Fixture f;
	f.selection().Assign(f.selection().Members(), f.b, f.a);
	CHECK(f.selection().Members() == std::vector<Entity>{f.a, f.b});
	CHECK(f.selection().Active() == f.b);
	CHECK(f.selection().Anchor() == f.a);
}

TEST_CASE("B10 selection prunes dead, foreign and parked entities") {
	Fixture f;
	Scene other = CreateScene(f.world, "Other");
	Entity foreign = other.CreateEntity("Foreign");
	Entity parked = f.scene.CreateEntity("Parked");
	Entity graveyard(f.a.Raw().world().entity());
	f.selection().Assign({f.a, f.b, foreign, parked}, parked);
	parked.SetParent(graveyard);
	f.b.Destroy();
	f.selection().Prune(f.scene.Root());
	CHECK(f.selection().Members() == std::vector<Entity>{f.a});
	CHECK(f.selection().Active() == f.a);
	graveyard.Destroy();
	UnloadScene(other, f.resources);
}

TEST_CASE("B10 root reduction keeps explicit children selected") {
	Fixture f;
	Entity child = f.a.CreateChild("Child");
	f.selection().Assign({child, f.b, f.a, child}, child);
	CHECK(SelectionRoots(f.selection().Members()) == std::vector<Entity>{f.b, f.a});
	CHECK(f.selection().Members().size() == 3);
}

TEST_CASE("B10 document bookmarks restore groups and locked descendants after snapshots") {
	Fixture f;
	Entity inst = f.scene.CreateEntity("Instance");
	inst.Add(SceneInstance{});
	Entity inherited = inst.CreateChild("Child");
	f.selection().Assign({f.a, inherited, f.b}, inherited, f.a);
	UpdateSceneDocumentStates(f.states, {f.scene});
	REQUIRE_FALSE(inherited.Has<EditorEntityId>());
	REQUIRE(TakeSnapshot(f.scene, f.resources));
	REQUIRE(RestoreSnapshot(f.scene, f.resources));
	UpdateSceneDocumentStates(f.states, {f.scene});
	CHECK(f.selection().Members().size() == 3);
	CHECK(f.selection().Active() == f.scene.Root().Find("Instance/Child"));
	CHECK(f.selection().Anchor() == f.scene.Root().Find("A"));
	CHECK_FALSE(f.a.IsAlive());
}

TEST_CASE("B10 selections remain independent across tabs and repeated play sessions") {
	World world;
	ResourceRegistry resources;
	FileSystem fs;
	SceneManager scenes(world, resources, fs);
	Scene first = scenes.Create("First"), second = scenes.Create("Second");
	Entity a = first.CreateEntity("A"), b = first.CreateEntity("B"), x = second.CreateEntity("X");
	std::vector<SceneDocumentSaveState> states;
	UpdateSceneDocumentStates(states, scenes.EditorScenes());
	FindSceneDocumentState(states, first)->context->selection.Assign({a, b}, a);
	FindSceneDocumentState(states, second)->context->selection.Set(x);
	UpdateSceneDocumentStates(states, scenes.EditorScenes());
	for (int i = 0; i < 2; ++i) {
		REQUIRE(scenes.SetCurrent(first));
		scenes.PlayCurrent();
		scenes.Flush();
		scenes.Pause();
		scenes.Flush();
		scenes.Stop();
		scenes.Flush();
		UpdateSceneDocumentStates(states, scenes.EditorScenes());
		first = scenes.EditorCurrent();
		second = Scene(world.Find("Second"));
		const auto &selection = FindSceneDocumentState(states, first)->context->selection;
		CHECK(selection.Members().size() == 2);
		CHECK(selection.Active() == first.Root().Find("A"));
		CHECK(FindSceneDocumentState(states, second)->context->selection.Active() ==
			second.Root().Find("X"));
	}
	states.clear();
}

TEST_CASE("B10 duplication remaps references between roots and assigns fresh subtree IDs") {
	Fixture f;
	Entity child = f.a.CreateChild("Child");
	child.Add(Links{f.b, {}, {}, false, {}});
	f.selection().Assign({child, f.a, f.b}, child);
	UpdateSceneDocumentStates(f.states, {f.scene});
	const u64 id = EnsureEditorEntityId(f.state(), child);
	REQUIRE(ExecuteDuplicateSelection(f.states, f.scene));
	Entity copyChild = f.selection().Active(), copyA = copyChild.GetParent(),
		   copyB = f.scene.Root().Find("B2");
	REQUIRE(copyB.IsAlive());
	CHECK(copyA.Get<Links>()->target == copyB);
	CHECK(copyB.Get<Links>()->target == copyA);
	CHECK(copyChild.Get<Links>()->target == copyB);
	CHECK(copyChild.Get<EditorEntityId>()->Value() != id);
	CHECK(f.selection().Members().size() == 3);
	REQUIRE(UndoSceneDocument(f.states, f.scene));
	CHECK_FALSE(f.scene.Root().Find("A2").IsAlive());
	CHECK_FALSE(f.scene.Root().Find("B2").IsAlive());
	CHECK(f.selection().Active() == child);
	CHECK_FALSE(f.state().history.IsDirty());
	REQUIRE(RedoSceneDocument(f.states, f.scene));
	CHECK(f.selection().Active() == copyChild);
	REQUIRE(UndoSceneDocument(f.states, f.scene));
	f.state().history.Clear();
	CHECK_FALSE(copyA.IsAlive());
	CHECK_FALSE(copyB.IsAlive());
}

TEST_CASE("B10 bulk delete restores mutual and incoming references and sibling order") {
	Fixture f;
	Entity c = f.scene.CreateEntity("C"), child = f.a.CreateChild("Child");
	c.Add(Links{child, {}, {}, false, {}});
	const auto order = f.scene.Root().GetChildren();
	f.selection().Assign({f.b, child, f.a}, child);
	REQUIRE(ExecuteDeleteSelection(f.states, f.scene));
	CHECK_FALSE(c.Get<Links>()->target.IsAlive());
	CHECK_FALSE(f.scene.Root().Find("A").IsAlive());
	CHECK_FALSE(f.scene.Root().Find("B").IsAlive());
	REQUIRE(UndoSceneDocument(f.states, f.scene));
	CHECK(f.a.Get<Links>()->target == f.b);
	CHECK(f.b.Get<Links>()->target == f.a);
	CHECK(c.Get<Links>()->target == child);
	CHECK(f.scene.Root().GetChildren() == order);
	CHECK(f.selection().Active() == child);
	CHECK_FALSE(f.state().history.IsDirty());
	REQUIRE(RedoSceneDocument(f.states, f.scene));
}

TEST_CASE("B10 bulk actions reject locked selections, roots, foreign destinations and cycles") {
	Fixture f;
	Entity inst = f.scene.CreateEntity("Instance");
	inst.Add(SceneInstance{});
	Entity child = inst.CreateChild("Child");
	f.selection().Assign({f.a, child});
	CHECK_FALSE(ExecuteDuplicateSelection(f.states, f.scene));
	CHECK_FALSE(ExecuteDeleteSelection(f.states, f.scene));
	f.selection().Assign({f.a, f.scene.Root()});
	CHECK_FALSE(ExecuteDeleteSelection(f.states, f.scene));
	f.selection().Assign({f.a, f.b});
	CHECK_FALSE(CanReparentSelection(f.selection(), f.scene, f.a));
	CHECK_FALSE(CanReparentSelection(f.selection(), f.scene, inst));
	CHECK_FALSE(f.state().history.IsDirty());
}

TEST_CASE("B10 structural actions reject stale members alongside live roots") {
	Fixture f;
	f.a.Destroy();
	REQUIRE_FALSE(CanEditSelection(f.selection(), f.scene, true));
	CHECK_FALSE(ExecuteDuplicateSelection(f.states, f.scene));
	CHECK_FALSE(ExecuteDeleteSelection(f.states, f.scene));
	CHECK_FALSE(ExecuteReparentSelection(f.states, f.scene, f.scene.Root()));
	CHECK(f.b.GetParent() == f.scene.Root());
	CHECK_FALSE(f.state().history.IsDirty());
}

TEST_CASE("B10 group reparent keeps world placement with one history step") {
	Fixture f;
	Entity parent = f.scene.CreateEntity("Parent");
	parent.Add(Transform2D{{20, 10}, 25, {2, 2}});
	f.a.Get<Transform2D>()->position = {10, 3};
	f.b.Get<Transform2D>()->position = {20, 4};
	const auto a = ComputeWorldMatrix(f.a), b = ComputeWorldMatrix(f.b);
	const auto order = f.scene.Root().GetChildren();
	REQUIRE(ExecuteReparentSelection(f.states, f.scene, parent));
	matrixNear(ComputeWorldMatrix(f.a), a);
	matrixNear(ComputeWorldMatrix(f.b), b);
	REQUIRE(UndoSceneDocument(f.states, f.scene));
	CHECK(f.scene.Root().GetChildren() == order);
	CHECK_FALSE(f.state().history.IsDirty());
	REQUIRE(RedoSceneDocument(f.states, f.scene));
	CHECK(f.a.GetParent() == parent);
	CHECK(f.b.GetParent() == parent);
	parent.Get<Transform2D>()->scale = {0, 1};
	CHECK_FALSE(CanReparentSelection(f.selection(), f.scene, f.scene.Root()));
}

TEST_CASE("B10 group transforms apply once to roots from a frozen baseline") {
	Fixture f;
	f.a.Get<Transform2D>()->position = {10, 0};
	f.b.Get<Transform2D>()->position = {20, 0};
	f.a.Add(SceneInstance{});
	Entity child = f.a.CreateChild("Child");
	child.Add(Transform2D{{2, 0}, 0, {1, 1}});
	f.selection().Assign({f.a, child, f.b}, f.b);
	CHECK_FALSE(CanEditSelection(f.selection(), f.scene, false));
	auto group = CaptureGroupTransform(f.selection(), false, false);
	REQUIRE(group);
	CHECK(group->targets.size() == 2);
	CHECK(group->pivot == Vector2(15, 0));
	const GroupTransformDelta delta{{5, 7}};
	auto values = TransformGroup(*group, delta);
	REQUIRE(values);
	REQUIRE(ApplyGroupTransform(*group, *values));
	CHECK(ComputeWorldMatrix(child)[3][0] == doctest::Approx(17));
	REQUIRE(ApplyGroupTransform(*group, *values));
	CHECK(f.a.Get<Transform2D>()->position == Vector2(15, 7));
	CHECK(child.Get<Transform2D>()->position == Vector2(2, 0));
	auto activePivot = CaptureGroupTransform(f.selection(), true, true);
	REQUIRE(activePivot);
	CHECK(activePivot->pivot == Vector2(25, 7));
	f.selection().Assign({child, f.b});
	auto mixed = CaptureGroupTransform(f.selection(), false, false);
	REQUIRE(mixed);
	CHECK(mixed->targets.size() == 1);
	CHECK(mixed->targets.front().entity == f.b);
	f.selection().Set(child);
	CHECK_FALSE(CaptureGroupTransform(f.selection(), false, false));
}

TEST_CASE("B10 transform representability rejects shear and accepts reflections") {
	auto reflected = ExactTransform2D(Matrix::CalculateTransform2D({4, 8}, 30, {-2, 3}));
	REQUIRE(reflected);
	matrixNear(reflected->LocalMatrix(), Matrix::CalculateTransform2D({4, 8}, 30, {-2, 3}));
	glm::mat4 shear(1);
	shear[1][0] = 0.5f;
	CHECK_FALSE(ExactTransform2D(shear));
	CHECK_FALSE(ExactTransform2D(glm::scale(glm::mat4(1), glm::vec3(0, 1, 1))));
}

TEST_CASE("B10 group movement is a shared world offset through different parents") {
	Fixture f;
	Entity parent = f.scene.CreateEntity("Parent");
	parent.Add(Transform2D{{18, -7}, 37, {-2, 3}});
	f.a.SetParent(parent);
	f.a.Add(Transform2D{{8, 2}, 15, {-4, 0}});
	f.b.Add(Transform2D{{-20, 6}, -40, {2, 7}});
	const auto aBefore = *f.a.Get<Transform2D>(), bBefore = *f.b.Get<Transform2D>();
	const auto aWorld = ComputeWorldMatrix(f.a), bWorld = ComputeWorldMatrix(f.b);
	auto group = CaptureGroupTransform(f.selection(), false, true);
	REQUIRE(group);
	auto values = TransformGroup(*group, {});
	REQUIRE(values);
	CHECK(values->front().position == aBefore.position);
	CHECK(values->back().position == bBefore.position);
	values = TransformGroup(*group, {{13, -9}});
	REQUIRE(values);
	REQUIRE(ApplyGroupTransform(*group, *values));
	for (auto pair : {std::pair{f.a, aWorld}, std::pair{f.b, bWorld}}) {
		const auto world = ComputeWorldMatrix(pair.first);
		CHECK(world[3][0] == doctest::Approx(pair.second[3][0] + 13));
		CHECK(world[3][1] == doctest::Approx(pair.second[3][1] - 9));
	}
	CHECK(f.a.Get<Transform2D>()->rotation == aBefore.rotation);
	CHECK(f.a.Get<Transform2D>()->scale == aBefore.scale);
	CHECK(f.b.Get<Transform2D>()->rotation == bBefore.rotation);
	CHECK(f.b.Get<Transform2D>()->scale == bBefore.scale);
}

TEST_CASE("B10 group rotation orbits the shared pivot under a nonuniform parent") {
	Fixture f;
	Entity parent = f.scene.CreateEntity("Parent");
	parent.Add(Transform2D{{10, 4}, 0, {2, 1}});
	f.a.SetParent(parent);
	f.a.Get<Transform2D>()->position = {5, 0};
	f.b.Get<Transform2D>()->position = {40, 4};
	auto group = CaptureGroupTransform(f.selection(), false, false);
	REQUIRE(group);
	CHECK(group->pivot == Vector2(30, 4));
	auto values = TransformGroup(*group, {{}, glm::radians(90.f)});
	REQUIRE(values);
	REQUIRE(ApplyGroupTransform(*group, *values));
	const auto a = ComputeWorldMatrix(f.a), b = ComputeWorldMatrix(f.b);
	CHECK(a[3][0] == doctest::Approx(30));
	CHECK(a[3][1] == doctest::Approx(-6));
	CHECK(b[3][0] == doctest::Approx(30));
	CHECK(b[3][1] == doctest::Approx(14));
	CHECK(f.a.Get<Transform2D>()->rotation == doctest::Approx(90));
	CHECK(f.b.Get<Transform2D>()->rotation == doctest::Approx(90));
	CHECK(f.a.Get<Transform2D>()->scale == Vector2(1));

	group = CaptureGroupTransform(f.selection(), true, false);
	REQUIRE(group);
	const auto pivot = ComputeWorldMatrix(f.a)[3];
	values = TransformGroup(*group, {{}, glm::radians(45.f)});
	REQUIRE(values);
	REQUIRE(ApplyGroupTransform(*group, *values));
	CHECK(ComputeWorldMatrix(f.a)[3][0] == doctest::Approx(pivot.x));
	CHECK(ComputeWorldMatrix(f.a)[3][1] == doctest::Approx(pivot.y));
}

TEST_CASE("B10 group axis scaling uses world and local bases on rotated entities") {
	Fixture f;
	f.a.Add(Transform2D{{10, 4}, 90, {-2, 3}});
	f.b.Add(Transform2D{{30, 4}, 0, {4, 2}});
	auto group = CaptureGroupTransform(f.selection(), false, false);
	REQUIRE(group);
	auto values = TransformGroup(*group, {{}, 0, {2, 1}});
	REQUIRE(values);
	REQUIRE(ApplyGroupTransform(*group, *values));
	CHECK(f.a.Get<Transform2D>()->position == Vector2(0, 4));
	CHECK(f.b.Get<Transform2D>()->position == Vector2(40, 4));
	CHECK(f.a.Get<Transform2D>()->scale.x == doctest::Approx(-2));
	CHECK(f.a.Get<Transform2D>()->scale.y == doctest::Approx(6));
	CHECK(f.b.Get<Transform2D>()->scale.x == doctest::Approx(8));
	CHECK(f.b.Get<Transform2D>()->scale.y == doctest::Approx(2));
	CHECK(f.a.Get<Transform2D>()->rotation == doctest::Approx(90));
	CHECK(f.b.Get<Transform2D>()->rotation == doctest::Approx(0));

	// Local X follows the active entity's rotated axis for every target.
	group = CaptureGroupTransform(f.selection(), false, true);
	REQUIRE(group);
	values = TransformGroup(*group, {{}, 0, {2, 1}});
	REQUIRE(values);
	CHECK(values->front().scale.x == doctest::Approx(-4));
	CHECK(values->front().scale.y == doctest::Approx(6));
	CHECK(values->back().scale.x == doctest::Approx(8));
	CHECK(values->back().scale.y == doctest::Approx(4));
}

TEST_CASE("B10 group axis scaling maps through parents and preserves the stretched X basis") {
	Fixture f;
	Entity parent = f.scene.CreateEntity("Parent");
	parent.Add(Transform2D{{12, 6}, 23, {2, 3}});
	f.a.SetParent(parent);
	f.a.Add(Transform2D{{10, 4}, 35, {-2, 3}});
	f.b.Add(Transform2D{{30, 4}, -20, {4, 2}});
	const auto aBefore = ComputeWorldMatrix(f.a), bBefore = ComputeWorldMatrix(f.b);
	auto group = CaptureGroupTransform(f.selection(), false, false);
	REQUIRE(group);
	for (Vector2 scale : {Vector2(2, 1), Vector2(1, 2)}) {
		auto values = TransformGroup(*group, {{}, 0, scale});
		REQUIRE(values);
		REQUIRE(ApplyGroupTransform(*group, *values));
		for (auto pair : {std::pair{f.a, aBefore}, std::pair{f.b, bBefore}}) {
			const auto world = ComputeWorldMatrix(pair.first);
			CHECK(world[0][0] == doctest::Approx(pair.second[0][0] * scale.x));
			CHECK(world[0][1] == doctest::Approx(pair.second[0][1] * scale.y));
			const f32 beforeArea =
				pair.second[0][0] * pair.second[1][1] - pair.second[1][0] * pair.second[0][1];
			const f32 afterArea = world[0][0] * world[1][1] - world[1][0] * world[0][1];
			CHECK(afterArea == doctest::Approx(beforeArea * scale.x * scale.y));
		}
	}
}

TEST_CASE("B10 mixed unsupported selections keep the editable transform gizmo") {
	Fixture f;
	Entity instance = f.scene.CreateEntity("Instance");
	instance.Add(SceneInstance{});
	Entity inherited = instance.CreateChild("Inherited");
	inherited.Add(Transform2D{});
	Entity ui = f.scene.CreateEntity("UI");
	ui.Add(Transform2D{});
	ui.Add(UIRect{});
	Entity parent = f.scene.CreateEntity("Collapsed");
	parent.Add(Transform2D{{}, 0, {0, 1}});
	Entity collapsed = parent.CreateChild("Child");
	collapsed.Add(Transform2D{});
	f.selection().Assign({inherited, f.a, collapsed, ui, f.b}, ui);
	auto group = CaptureGroupTransform(f.selection(), false, false);
	REQUIRE(group);
	REQUIRE(group->targets.size() == 2);
	auto values = TransformGroup(*group, {{5, 7}});
	REQUIRE(values);
	REQUIRE(ApplyGroupTransform(*group, *values));
	CHECK(f.a.Get<Transform2D>()->position == Vector2(5, 7));
	CHECK(f.b.Get<Transform2D>()->position == Vector2(5, 7));
	CHECK(inherited.Get<Transform2D>()->position == Vector2(0));
	CHECK(collapsed.Get<Transform2D>()->position == Vector2(0));
	CHECK(ui.Get<Transform2D>()->position == Vector2(0));
}

TEST_CASE("B10 transformless selection roots move their transform descendants once") {
	Fixture f;
	Entity folder = f.scene.CreateEntity("Folder");
	f.a.SetParent(folder);
	f.a.Get<Transform2D>()->position = {10, 0};
	f.selection().Assign({folder, f.a, f.b}, f.a);
	auto group = CaptureGroupTransform(f.selection(), false, false);
	REQUIRE(group);
	REQUIRE(group->targets.size() == 2);
	auto values = TransformGroup(*group, {{7, 9}});
	REQUIRE(values);
	REQUIRE(ApplyGroupTransform(*group, *values));
	CHECK(f.a.Get<Transform2D>()->position == Vector2(17, 9));
	CHECK(f.b.Get<Transform2D>()->position == Vector2(7, 9));
	CHECK_FALSE(folder.Has<Transform2D>());
}

TEST_CASE("B10 active pivot and local axes use the selected ancestor of an active child") {
	Fixture f;
	f.a.Add(SceneInstance{});
	f.a.Add(Transform2D{{10, 20}, 35, {1, 1}});
	f.b.Get<Transform2D>()->position = {70, 20};
	Entity child = f.a.CreateChild("Child");
	child.Add(Transform2D{{100, 40}, -60, {1, 1}});
	f.selection().Assign({child, f.b, f.a}, child);
	CHECK(f.selection().Active() == child);
	CHECK(f.selection().ActiveRoot() == f.a);
	auto group = CaptureGroupTransform(f.selection(), true, true);
	REQUIRE(group);
	CHECK(group->pivot == Vector2(10, 20));
	CHECK(group->angle == doctest::Approx(glm::radians(35.f)));
	auto values = TransformGroup(*group, {{}, glm::radians(90.f)});
	REQUIRE(values);
	REQUIRE(ApplyGroupTransform(*group, *values));
	CHECK(f.a.Get<Transform2D>()->position == Vector2(10, 20));
	CHECK(f.b.Get<Transform2D>()->position.x == doctest::Approx(10));
	CHECK(f.b.Get<Transform2D>()->position.y == doctest::Approx(80));
	CHECK(child.Get<Transform2D>()->position == Vector2(100, 40));

	// An independently selected locked child is not a pivot for another editable root.
	f.selection().Assign({f.b, child}, child);
	group = CaptureGroupTransform(f.selection(), true, false);
	REQUIRE(group);
	CHECK(group->pivot == f.b.Get<Transform2D>()->position);
}

TEST_CASE("B10 inspector edits editable roots when selected instance children are locked") {
	Fixture f;
	f.a.Add(SceneInstance{});
	Entity child = f.a.CreateChild("Child");
	child.Add(Transform2D{{3, 4}, 0, {1, 1}});
	f.selection().Assign({f.a, child, f.b}, child);
	auto targets = InspectorTargets(f.selection());
	CHECK(targets.Members() == std::vector<Entity>{f.a, f.b});
	CHECK(targets.Active() == f.a);
	CHECK(CanEditSelection(targets, f.scene));
	const auto shared = SharedInspectorFields(targets);
	CHECK(std::any_of(shared.begin(), shared.end(), [](const auto &field) {
		return field.component == "Links" && field.field.name == "point";
	}));

	auto edits = AssignBulkValue(ReadBulkField(targets, f.field("point")), Vector2(9, 99), 1);
	REQUIRE(ExecuteBulkFieldEdit(f.states, f.scene, edits));
	CHECK(f.a.Get<Links>()->point == Vector2(9, 2));
	CHECK(f.b.Get<Links>()->point == Vector2(9, 4));
	CHECK_FALSE(child.Has<Links>());
	REQUIRE(UndoSceneDocument(f.states, f.scene));
	CHECK(f.a.Get<Links>()->point == Vector2(1, 2));
	CHECK(f.b.Get<Links>()->point == Vector2(3, 4));
	CHECK(f.selection().Active() == child);

	const SharedInspectorField position{"Transform2D",
		*f.world.FindComponent("Transform2D").FindField("position")};
	edits = AssignBulkValue(ReadBulkField(targets, position), Vector2(100, 200));
	REQUIRE(ExecuteBulkFieldEdit(f.states, f.scene, edits));
	CHECK(f.a.Get<Transform2D>()->position == Vector2(100, 200));
	CHECK(f.b.Get<Transform2D>()->position == Vector2(100, 200));
	CHECK(child.Get<Transform2D>()->position == Vector2(3, 4));
	CHECK(ComputeWorldMatrix(child)[3][0] == doctest::Approx(103));
	CHECK(ComputeWorldMatrix(child)[3][1] == doctest::Approx(204));
	REQUIRE(UndoSceneDocument(f.states, f.scene));
	CHECK_FALSE(f.state().history.IsDirty());

	f.selection().Assign({child, f.b}, child);
	targets = InspectorTargets(f.selection());
	CHECK(targets.Members() == std::vector<Entity>{f.b});
	f.selection().Set(child);
	CHECK(InspectorTargets(f.selection()).Members().empty());
}

TEST_CASE("B10 inspector reduces editable descendants and structural actions use hierarchy roots") {
	Fixture f;
	Entity child = f.a.CreateChild("Child");
	child.Add(Transform2D{{3, 4}, 0, {1, 1}});
	f.selection().Assign({child, f.a, f.b}, child);
	CHECK(InspectorTargets(f.selection()).Members() == std::vector<Entity>{f.a, f.b});
	f.a.Add(SceneInstance{});
	CHECK(CanEditSelection(f.selection(), f.scene, true));

	SUBCASE("duplicate includes the selected inherited child without duplicating it twice") {
		REQUIRE(ExecuteDuplicateSelection(f.states, f.scene));
		CHECK(SelectionRoots(f.selection().Members()).size() == 2);
		CHECK(f.selection().Members().size() == 3);
		REQUIRE(UndoSceneDocument(f.states, f.scene));
		CHECK(f.selection().Active() == child);
	}
	SUBCASE("delete and undo preserve the whole selected hierarchy") {
		REQUIRE(ExecuteDeleteSelection(f.states, f.scene));
		CHECK_FALSE(f.a.IsDescendantOf(f.scene.Root()));
		REQUIRE(UndoSceneDocument(f.states, f.scene));
		CHECK(f.a.IsDescendantOf(f.scene.Root()));
		CHECK(child.GetParent() == f.a);
		CHECK(f.selection().Active() == child);
	}
	SUBCASE("reparent changes the parent root and leaves its inherited child attached") {
		Entity destination = f.scene.CreateEntity("Destination");
		REQUIRE(ExecuteReparentSelection(f.states, f.scene, destination));
		CHECK(f.a.GetParent() == destination);
		CHECK(child.GetParent() == f.a);
		REQUIRE(UndoSceneDocument(f.states, f.scene));
		CHECK(f.a.GetParent() == f.scene.Root());
	}
	SUBCASE("dead selections do not enable structural actions") {
		f.selection().Set(f.a);
		f.a.Destroy();
		REQUIRE_FALSE(CanEditSelection(f.selection(), f.scene, true));
		CHECK_FALSE(ExecuteDeleteSelection(f.states, f.scene));
		CHECK_FALSE(ExecuteDuplicateSelection(f.states, f.scene));
	}
}

TEST_CASE("B10 bulk fields preserve mixed channels and restore distinct originals") {
	Fixture f;
	auto fields = SharedInspectorFields(f.selection());
	CHECK_FALSE(fields.empty());
	auto values = ReadBulkField(f.selection(), f.field("point"));
	REQUIRE(values.size() == 2);
	auto edits = AssignBulkValue(values, Vector2(9, 99), 1);
	REQUIRE(ExecuteBulkFieldEdit(f.states, f.scene, edits));
	CHECK(f.a.Get<Links>()->point == Vector2(9, 2));
	CHECK(f.b.Get<Links>()->point == Vector2(9, 4));
	REQUIRE(UndoSceneDocument(f.states, f.scene));
	CHECK(f.a.Get<Links>()->point == Vector2(1, 2));
	CHECK(f.b.Get<Links>()->point == Vector2(3, 4));
	CHECK_FALSE(f.state().history.IsDirty());
	edits = AssignBulkValue(ReadBulkField(f.selection(), f.field("color")),
		Color(0.25f, 0.5f, 0.75f, 1), 4);
	REQUIRE(ExecuteBulkFieldEdit(f.states, f.scene, edits));
	CHECK(f.a.Get<Links>()->color.r == 1);
	CHECK(f.b.Get<Links>()->color.a == 0.5f);
	CHECK(f.b.Get<Links>()->color.b == 0.75f);
}

TEST_CASE("B10 bulk field failure rolls back prior targets and does not advance history") {
	Fixture f;
	auto edits = AssignBulkValue(ReadBulkField(f.selection(), f.field("point")), Vector2(8, 9));
	REQUIRE(ExecuteBulkFieldEdit(f.states, f.scene, edits));
	// Undo runs backwards: B succeeds first, then missing A forces B's redo rollback.
	f.a.Remove<Links>();
	CHECK_FALSE(UndoSceneDocument(f.states, f.scene));
	CHECK(f.b.Get<Links>()->point == Vector2(8, 9));
	CHECK(f.state().history.CanUndo());
	f.a.Add(Links{});
	REQUIRE(UndoSceneDocument(f.states, f.scene));
	CHECK(f.b.Get<Links>()->point == Vector2(3, 4));
	CHECK_FALSE(f.state().history.IsDirty());
	// Redo changes A before finding B missing; A must be rolled back.
	f.b.Remove<Links>();
	CHECK_FALSE(RedoSceneDocument(f.states, f.scene));
	CHECK(f.a.Get<Links>()->point == Vector2(1, 2));
	CHECK_FALSE(f.state().history.IsDirty());
}

TEST_CASE("B10 previews cancel and no-op edits leave clean history") {
	Fixture f;
	auto values = ReadBulkField(f.selection(), f.field("point"));
	CHECK_FALSE(ExecuteBulkFieldEdit(f.states, f.scene, values));
	auto edits = AssignBulkValue(values, Vector2(8, 9));
	REQUIRE(PreviewBulkFieldEdit(edits));
	REQUIRE(PreviewBulkFieldEdit(edits, true));
	CHECK(f.a.Get<Links>()->point == Vector2(1, 2));
	CHECK(f.b.Get<Links>()->point == Vector2(3, 4));
	CHECK_FALSE(f.state().history.IsDirty());
	f.b.Remove<Links>();
	CHECK_FALSE(PreviewBulkFieldEdit(edits));
	CHECK(f.a.Get<Links>()->point == Vector2(1, 2));
	auto shared = SharedInspectorFields(f.selection());
	CHECK(std::none_of(shared.begin(), shared.end(),
		[](const auto &field) { return field.component == "Links"; }));
}

TEST_CASE("B10 one shared resource assignment retains a newly created resource through undo") {
	Fixture f;
	f.resources.RegisterType<ImageTexture>("ImageTexture");
	const auto texture = CreateSceneResource(f.scene, f.resources, "ImageTexture");
	REQUIRE(texture.IsValid());
	auto edits = AssignBulkValue(ReadBulkField(f.selection(), f.field("texture")), texture);
	REQUIRE(ExecuteBulkFieldEdit(f.states, f.scene, edits, &f.resources, texture));
	CHECK(f.a.Get<Links>()->texture == texture);
	CHECK(f.b.Get<Links>()->texture == texture);
	REQUIRE(UndoSceneDocument(f.states, f.scene));
	CHECK_FALSE(f.scene.Owns(texture));
	CHECK(f.resources.Get(texture));
	REQUIRE(RedoSceneDocument(f.states, f.scene));
	CHECK(f.scene.Owns(texture));
	REQUIRE(UndoSceneDocument(f.states, f.scene));
	f.state().history.Clear();
	CHECK_FALSE(f.resources.Get(texture));
}

TEST_CASE("B10 marquee clips normalized coordinates and flips the framebuffer Y axis") {
	auto rect = BoxPickRectangle({0.75f, 0.5f}, {0.25f, 0.1f}, {200, 100});
	REQUIRE(rect);
	CHECK(rect->x == 50);
	CHECK(rect->y == 50);
	CHECK(rect->width == 100);
	CHECK(rect->height == 40);
	auto clipped = BoxPickRectangle({-1, -1}, {2, 2}, {200, 100});
	REQUIRE(clipped);
	CHECK(clipped->width == 200);
	CHECK(clipped->height == 100);
	CHECK(clipped->y == 0);
	CHECK_FALSE(BoxPickRectangle({-1, 0}, {-0.2f, 1}, {200, 100}));
	CHECK_FALSE(BoxPickRectangle({0, 0}, {1, 1}, {0, 0}));
	CHECK_FALSE(BoxPickRectangle({std::numeric_limits<float>::quiet_NaN(), 0}, {1, 1}, {200, 100}));
}

TEST_CASE("B10 undo never resolves an assigned entity identity to a replacement at its old path") {
	Fixture f;
	auto edits = AssignBulkValue(ReadBulkField(f.selection(), f.field("point")), Vector2(8, 9));
	REQUIRE(ExecuteBulkFieldEdit(f.states, f.scene, edits));
	f.a.Destroy();
	Entity replacement = f.scene.CreateEntity("A");
	replacement.Add(Links{Entity{}, Vector2(77, 88), {}, false, {}});
	CHECK_FALSE(UndoSceneDocument(f.states, f.scene));
	CHECK(replacement.Get<Links>()->point == Vector2(77, 88));
	CHECK(f.b.Get<Links>()->point == Vector2(8, 9));
}

TEST_CASE("B10 restoring an empty destination child order is a valid no-op") {
	Fixture f;
	CHECK(f.a.SetChildOrder({}));
	Entity child = f.a.CreateChild("Child");
	CHECK_FALSE(f.a.SetChildOrder({}));
	child.SetParent(f.b);
	CHECK(f.a.SetChildOrder({}));
}

TEST_CASE("B10 bulk data-described fields rebind offsets and reject incompatible reloads") {
	Fixture f;
	auto registered = f.world.RegisterComponent("Dynamic",
		{{"value", FieldType::F32}, {"hidden", FieldType::I32, FieldFlags::None}});
	REQUIRE(registered);
	ComponentType type = registered.value;
	f.a.Add(type);
	f.b.Add(type);
	REQUIRE(f.a.GetField(type, "value").Set(2.f));
	REQUIRE(f.b.GetField(type, "value").Set(4.f));
	SharedInspectorField field{"Dynamic", *type.FindField("value")};
	auto edits = AssignBulkValue(ReadBulkField(f.selection(), field), 9.f);
	REQUIRE(ExecuteBulkFieldEdit(f.states, f.scene, edits));
	auto reordered = f.world.ReplaceDataDescribedFields(type,
		{{"padding", FieldType::String}, {"value", FieldType::F32}});
	REQUIRE(reordered);
	type = reordered.value;
	REQUIRE(UndoSceneDocument(f.states, f.scene));
	f32 a = 0, b = 0;
	REQUIRE(f.a.GetField(type, "value").Get(a));
	REQUIRE(f.b.GetField(type, "value").Get(b));
	CHECK(a == 2);
	CHECK(b == 4);
	REQUIRE(RedoSceneDocument(f.states, f.scene));
	auto replaced = f.world.ReplaceDataDescribedFields(type, {{"value", FieldType::I32}});
	REQUIRE(replaced);
	type = replaced.value;
	CHECK_FALSE(ReadBulkField(f.selection(), field).size());
	CHECK_FALSE(UndoSceneDocument(f.states, f.scene));
	CHECK_FALSE(PreviewBulkFieldEdit(edits));
	i32 current = -1;
	REQUIRE(f.a.GetField(type, "value").Get(current));
	CHECK(current == 0);
}

TEST_CASE("B10 incompatible resource and entity references are rejected before any field changes") {
	Fixture f;
	auto registered = f.world.RegisterComponent("TextureHolder",
		{{"texture", FieldType::ResourceRef, FieldFlags::Default, "", {}, "ImageTexture"}});
	REQUIRE(registered);
	ComponentType type = registered.value;
	f.a.Add(type);
	f.b.Add(type);
	f.resources.RegisterType<ImageTexture>("ImageTexture");

	struct WrongResource : Resource {};

	f.resources.RegisterType<WrongResource>("WrongResource");
	auto wrong = f.resources.Create<WrongResource>();
	REQUIRE(wrong);
	REQUIRE(f.scene.Own(f.resources, wrong->Id()));
	SharedInspectorField field{"TextureHolder", *type.FindField("texture")};
	auto edits = AssignBulkValue(ReadBulkField(f.selection(), field), wrong->Id());
	CHECK_FALSE(ExecuteBulkFieldEdit(f.states, f.scene, edits, &f.resources));
	CHECK_FALSE(f.state().history.IsDirty());
	World foreignWorld;
	Entity foreign = foreignWorld.CreateEntity();
	foreign.SetName("Foreign");
	edits = AssignBulkValue(ReadBulkField(f.selection(), f.field("target")), foreign);
	CHECK_FALSE(ExecuteBulkFieldEdit(f.states, f.scene, edits));
	CHECK(f.a.Get<Links>()->target == f.b);
	CHECK(f.b.Get<Links>()->target == f.a);
}

TEST_CASE("B10 active local basis and reflected parent reparenting preserve exact transforms") {
	Fixture f;
	f.a.Get<Transform2D>()->rotation = 45;
	f.a.Get<Transform2D>()->scale = {-2, 3};
	f.selection().Assign({f.b, f.a}, f.a);
	auto group = CaptureGroupTransform(f.selection(), true, true);
	REQUIRE(group);
	CHECK(group->angle ==
		doctest::Approx(std::atan2(ComputeWorldMatrix(f.a)[0][1], ComputeWorldMatrix(f.a)[0][0])));
	Entity parent = f.scene.CreateEntity("Reflected");
	parent.Add(Transform2D{{10, 20}, 30, {-2, 2}});
	const auto a = ComputeWorldMatrix(f.a), b = ComputeWorldMatrix(f.b);
	REQUIRE(ExecuteReparentSelection(f.states, f.scene, parent));
	matrixNear(ComputeWorldMatrix(f.a), a);
	matrixNear(ComputeWorldMatrix(f.b), b);
	REQUIRE(UndoSceneDocument(f.states, f.scene));
	matrixNear(ComputeWorldMatrix(f.a), a);
	matrixNear(ComputeWorldMatrix(f.b), b);
}
