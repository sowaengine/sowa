#include <doctest/doctest.h>

#include "ecs/components/editor_entity_id.hpp"
#include "ecs/components/transform2d.hpp"
#include "ecs/world.hpp"
#include "editor/inspector.hpp"
#include "editor/scene_commands.hpp"
#include "editor/scene_documents.hpp"
#include "resource/image_texture.hpp"
#include "scene/scene_serializer.hpp"
#include "scene/scene_snapshot.hpp"

using namespace sowa;

namespace {
struct CopyData {
	std::string label;
	Entity target;
	ResourceID texture;
};

struct OpaqueData {
	std::vector<std::string> entries;
};

struct ActionFixture {
	World world;
	ResourceRegistry resources;
	Scene scene = CreateScene(world, "Scene");
	std::vector<editor::SceneDocumentSaveState> states;

	ActionFixture() {
		world.RegisterComponent<CopyData>("CopyData")
			.Field("label", &CopyData::label)
			.Field("target", &CopyData::target)
			.Field("texture", &CopyData::texture);
		world.RegisterComponent<OpaqueData>("OpaqueData");
		editor::UpdateSceneDocumentStates(states, {scene});
	}
};
} // namespace

TEST_CASE("component removal restores opaque values and respects structural protection") {
	ActionFixture f;
	auto node = f.scene.CreateEntity("Node");
	node.Add(OpaqueData{{"one", "two"}});
	auto type = f.world.FindComponent("OpaqueData");
	REQUIRE(editor::ExecuteRemoveComponent(f.states, f.scene, node, type));
	CHECK_FALSE(node.Has(type));
	REQUIRE(editor::UndoSceneDocument(f.states, f.scene));
	REQUIRE(node.Get<OpaqueData>());
	CHECK(node.Get<OpaqueData>()->entries == std::vector<std::string>{"one", "two"});
	REQUIRE(editor::RedoSceneDocument(f.states, f.scene));
	CHECK_FALSE(node.Has(type));
	CHECK_FALSE(editor::ExecuteRemoveComponent(f.states, f.scene, f.scene.Root(),
		f.world.FindComponent("SceneRoot")));
	CHECK_FALSE(editor::CanRemoveComponent(node, type, true));
	auto instance = f.scene.CreateEntity("Instance");
	instance.Add(SceneInstance{});
	auto locked = instance.CreateChild("Locked");
	locked.Add<Transform2D>();
	CHECK_FALSE(editor::ExecuteRemoveComponent(f.states, f.scene, locked,
		f.world.FindComponent("Transform2D")));
}

TEST_CASE("duplicate is undoable with fresh identities and remapped child references") {
	ActionFixture f;
	auto original = f.scene.CreateEntity("Player");
	auto child = original.CreateChild("Arm");
	original.Add(CopyData{"original", child, {}});
	original.AddGroup("players");
	child.SetVisible(false);
	editor::EnsureEditorEntityId(f.states.front(), original);
	auto copy = editor::ExecuteDuplicateEntity(f.states, f.scene, original);
	REQUIRE(copy.IsAlive());
	CHECK(copy.GetName() == "Player2");
	CHECK(copy.Get<CopyData>()->target == copy.Find("Arm"));
	CHECK(copy.Get<EditorEntityId>()->Value() != original.Get<EditorEntityId>()->Value());
	CHECK(copy.IsInGroup("players"));
	CHECK_FALSE(copy.Find("Arm").IsVisible());
	REQUIRE(editor::UndoSceneDocument(f.states, f.scene));
	CHECK_FALSE(f.scene.Root().Find("Player2").IsAlive());
	REQUIRE(editor::RedoSceneDocument(f.states, f.scene));
	CHECK(f.scene.Root().Find("Player2") == copy);
	CHECK_FALSE(editor::ExecuteDuplicateEntity(f.states, f.scene, f.scene.Root()).IsAlive());
}

TEST_CASE("clipboard snapshots survive source deletion and paste repeatedly across documents") {
	ActionFixture f;
	f.resources.RegisterType<ImageTexture>("ImageTexture").Field("path", &ImageTexture::path);
	auto texture = f.resources.Create<ImageTexture>();
	REQUIRE(f.scene.Own(f.resources, texture->Id()));
	auto original = f.scene.CreateEntity("Player");
	original.Add(CopyData{"copied", original.CreateChild("Arm"), texture->Id()});
	original.SetVisible(false);
	const auto text = editor::CopyEntity(original, f.resources);
	REQUIRE(editor::IsEntityClipboard(text));
	original.Get<CopyData>()->label = "changed";
	original.Destroy();
	auto destination = CreateScene(f.world, "Other");
	editor::UpdateSceneDocumentStates(f.states, {f.scene, destination});
	auto copy = editor::ExecutePasteEntity(f.states, destination, destination.Root(), text, f.world,
		f.resources);
	REQUIRE(copy.IsAlive());
	CHECK(copy.Get<CopyData>()->label == "copied");
	CHECK(copy.Get<CopyData>()->target == copy.Find("Arm"));
	CHECK_FALSE(copy.IsVisible());
	CHECK(destination.Owns(texture->Id()));
	REQUIRE(editor::UndoSceneDocument(f.states, destination));
	CHECK_FALSE(destination.Root().Find("Player").IsAlive());
	CHECK_FALSE(destination.Owns(texture->Id()));
	REQUIRE(editor::RedoSceneDocument(f.states, destination));
	CHECK(destination.Owns(texture->Id()));
	auto another = editor::ExecutePasteEntity(f.states, destination, destination.Root(), text,
		f.world, f.resources);
	REQUIRE(another.IsAlive());
	CHECK(another.GetName() == "Player2");
	CHECK(another.Get<CopyData>()->target == another.Find("Arm"));
	CHECK_FALSE(editor::ExecutePasteEntity(f.states, destination, destination.Root(), "plain text",
		f.world, f.resources)
			.IsAlive());
	f.states.clear();
	UnloadScene(destination, f.resources);
	UnloadScene(f.scene, f.resources);
}

TEST_CASE("component categories group builtins and custom components") {
	ActionFixture f;
	CHECK(editor::ComponentCategory(f.world.FindComponent("Transform2D")) == "General");
	CHECK(editor::ComponentCategory(f.world.FindComponent("CopyData")) == "Gameplay and Scripts");
}

TEST_CASE("removed component references follow entities restored after play") {
	ActionFixture f;
	auto holder = f.scene.CreateEntity("Holder");
	auto target = f.scene.CreateEntity("Target");
	holder.Add(CopyData{"saved", target, {}});
	REQUIRE(editor::ExecuteRemoveComponent(f.states, f.scene, holder,
		f.world.FindComponent("CopyData")));
	REQUIRE(TakeSnapshot(f.scene, f.resources));
	REQUIRE(RestoreSnapshot(f.scene, f.resources));
	editor::UpdateSceneDocumentStates(f.states, {f.scene});
	REQUIRE(editor::UndoSceneDocument(f.states, f.scene));
	auto restored = f.scene.Root().Find("Holder");
	REQUIRE(restored.Get<CopyData>());
	CHECK(restored.Get<CopyData>()->target == f.scene.Root().Find("Target"));
	CHECK(restored.Get<CopyData>()->target.IsAlive());
}

TEST_CASE("clipboard validates fragments before creating entities") {
	ActionFixture f;
	const auto before = GetScenes(f.world).size();
	CHECK_FALSE(editor::ExecutePasteEntity(f.states, f.scene, f.scene.Root(),
		"# Sowa Entity Clipboard v1\nroot: {name: Invalid}", f.world, f.resources)
			.IsAlive());
	CHECK(GetScenes(f.world).size() == before);
	CHECK(f.scene.Root().GetChildren().empty());
	CHECK_FALSE(f.states.front().history.CanUndo());
}
