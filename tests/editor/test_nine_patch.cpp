#include <doctest/doctest.h>

#include "ecs/world.hpp"
#include "editor/nine_patch_tools.hpp"
#include "editor/scene_commands.hpp"
#include "scene/scene.hpp"

using namespace sowa;

TEST_CASE("Nine-patch editor: every resize corner keeps its opposite fixed") {
	for (bool centered : {false, true}) {
		for (const Vector2 corner : {Vector2(0, 0), Vector2(1, 0), Vector2(1, 1), Vector2(0, 1)}) {
			NinePatchRect before;
			before.size = {100, 80};
			before.offset = {12, -7};
			before.centered = centered;
			const auto after = editor::ResizeNinePatchRect(before, {20, -10}, corner);
			const auto a = before.LocalRect(), b = after.LocalRect();
			CHECK(b.x + (1 - corner.x) * b.w == doctest::Approx(a.x + (1 - corner.x) * a.w));
			CHECK(b.y + (1 - corner.y) * b.h == doctest::Approx(a.y + (1 - corner.y) * a.h));
			CHECK(b.x + corner.x * b.w == doctest::Approx(a.x + corner.x * a.w + 20));
			CHECK(b.y + corner.y * b.h == doctest::Approx(a.y + corner.y * a.h - 10));
		}
	}
	NinePatchRect before;
	const auto small = editor::ResizeNinePatchRect(before, {1000, 1000}, {0, 0});
	CHECK(small.size == Vector2(1, 1));
	CHECK(small.LocalRect().Right() == before.LocalRect().Right());
	CHECK(small.LocalRect().Top() == before.LocalRect().Top());
}

TEST_CASE("Nine-patch editor: texture guides round pixels and cannot cross the opposite padding") {
	CHECK(editor::NinePatchPaddingAt("paddingLeft", {12.4f, 99}, {64, 32}, 8) == 12);
	CHECK(editor::NinePatchPaddingAt("paddingRight", {12.4f, 99}, {64, 32}, 8) == 52);
	CHECK(editor::NinePatchPaddingAt("paddingTop", {99, 6.7f}, {64, 32}, 8) == 7);
	CHECK(editor::NinePatchPaddingAt("paddingBottom", {99, 6.7f}, {64, 32}, 8) == 24);
	CHECK(editor::NinePatchPaddingAt("paddingLeft", {100, 0}, {64, 32}, 8) == 56);
	CHECK(editor::NinePatchPaddingAt("paddingLeft", {100, 0}, {64, 32}, -10) == 64);
	CHECK(editor::NinePatchPaddingAt("paddingTop", {0, -10}, {64, 32}, 8) == 0);
}

TEST_CASE("Nine-patch editor: a resize is one undoable operation and fields expose only padding") {
	World world;
	const Scene scene = CreateScene(world, "NinePatchTest");
	Entity entity = editor::SpawnChild(scene.Root(), editor::EntityArchetype::NinePatchRect);
	std::vector<editor::SceneDocumentSaveState> states;
	editor::UpdateSceneDocumentStates(states, {scene});
	const auto before = *entity.Get<NinePatchRect>();
	const auto after = editor::ResizeNinePatchRect(before, {30, 15}, {0, 1});
	*entity.Get<NinePatchRect>() = after;
	REQUIRE(editor::RecordNinePatchResize(states, scene, entity, before, after));
	REQUIRE(editor::UndoSceneDocument(states, scene));
	CHECK(entity.Get<NinePatchRect>()->size == before.size);
	CHECK(entity.Get<NinePatchRect>()->offset == before.offset);
	REQUIRE(editor::RedoSceneDocument(states, scene));
	CHECK(entity.Get<NinePatchRect>()->size == after.size);
	CHECK(entity.Get<NinePatchRect>()->offset == after.offset);
	for (const auto *name : {"NinePatchRect", "UINinePatchRect"}) {
		const auto type = world.FindComponent(name);
		CHECK(type.FindField("paddingLeft"));
		CHECK(type.FindField("paddingRight"));
		CHECK(type.FindField("paddingTop"));
		CHECK(type.FindField("paddingBottom"));
		CHECK_FALSE(type.FindField("borderMin"));
		CHECK_FALSE(type.FindField("borderMax"));
		CHECK_FALSE(type.FindField("regionPosition"));
		CHECK_FALSE(type.FindField("regionSize"));
	}
}
