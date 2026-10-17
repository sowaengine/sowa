#include <doctest/doctest.h>

#include <cstddef>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "core/filesystem/file_system.hpp"
#include "core/log.hpp"
#include "ecs/components/transform2d.hpp"
#include "ecs/world.hpp"
#include "editor/filesystem_browser.hpp"
#include "editor/scene_commands.hpp"
#include "editor/scene_documents.hpp"
#include "imgui.h"
#include "imgui_internal.h"
#include "resource/resource_registry.hpp"
#include "scene/scene_manager.hpp"

using namespace sowa;

namespace {

struct Fixture {
	World world;
	ResourceRegistry resources;
	FileSystem fs;
	SceneManager scenes{world, resources, fs};
};

class CounterCommand final : public editor::EditCommand {
  public:
	CounterCommand(int &value, int &discarded) : _value(&value), _discarded(&discarded) {
	}

	std::string_view Name() const override {
		return "Increment";
	}

	bool Undo() override {
		--*_value;
		return true;
	}

	bool Redo() override {
		++*_value;
		return true;
	}

	void Discard() override {
		++*_discarded;
	}

  private:
	int *_value = nullptr;
	int *_discarded = nullptr;
};

} // namespace

TEST_CASE("command history bounds retained undo state") {
	int value = 0;
	int discarded = 0;
	editor::CommandHistory history(true, 2);
	for (int i = 0; i < 3; ++i) {
		REQUIRE(history.Execute(std::make_unique<CounterCommand>(value, discarded)));
	}
	CHECK(value == 3);
	CHECK(discarded == 1);
	REQUIRE(history.Undo());
	REQUIRE(history.Undo());
	CHECK_FALSE(history.Undo());
	CHECK(value == 1);
}

TEST_CASE("activating a scene document pauses the other tabs") {
	Fixture f;
	Scene first = f.scenes.Create("First");
	Scene second = f.scenes.Create("Second");

	REQUIRE(editor::ActivateSceneDocument(f.scenes, first));
	CHECK(first.IsEnabled());
	CHECK_FALSE(second.IsEnabled());
	CHECK(f.scenes.Current().Root() == first.Root());

	REQUIRE(editor::ActivateSceneDocument(f.scenes, second));
	CHECK_FALSE(first.IsEnabled());
	CHECK(second.IsEnabled());
	CHECK(f.scenes.Current().Root() == second.Root());
}

TEST_CASE("activating an invalid scene document is refused") {
	Fixture f;
	Scene scene = f.scenes.Create("Scene");
	REQUIRE(editor::ActivateSceneDocument(f.scenes, scene));

	CHECK_FALSE(editor::ActivateSceneDocument(f.scenes, Scene()));
	CHECK(f.scenes.Current().Root() == scene.Root());
}

TEST_CASE("scene document state detects edits and clears after a logged save") {
	const std::filesystem::path dir =
		std::filesystem::temp_directory_path() / "sowa_scene_document_save_test";
	std::filesystem::remove_all(dir);
	std::filesystem::create_directories(dir);

	Fixture f;
	f.fs.MountFolder("tmp", dir);
	Scene scene = f.scenes.Create("Scene");
	scene.SetSourcePath("tmp://scene.scene");
	std::vector<editor::SceneDocumentSaveState> states;
	editor::UpdateSceneDocumentStates(states, {scene});
	CHECK_FALSE(editor::IsSceneDocumentUnsaved(states, scene));

	Entity child = editor::ExecuteCreateEntity(
		states, scene, scene.Root(), editor::EntityArchetype::Empty);
	REQUIRE(child.IsAlive());
	CHECK(editor::IsSceneDocumentUnsaved(states, scene));

	Log::Clear();
	REQUIRE(editor::SaveSceneDocument(f.fs, f.resources, scene, states));
	CHECK_FALSE(editor::IsSceneDocumentUnsaved(states, scene));
	const FsResult<std::shared_ptr<FileData>> saved =
		f.fs.Load("tmp://scene.scene");
	REQUIRE(saved);
	REQUIRE(saved.value != nullptr);
	CHECK(saved.value->Size() > 0);
	const std::string savedText(
		reinterpret_cast<const char *>(saved.value->Data()), saved.value->Size());
	CHECK(savedText.find("EditorEntityId") == std::string::npos);
	REQUIRE_FALSE(Log::GetLines().empty());
	CHECK(Log::GetLines().back().text.find(
			  "Saved scene 'Scene' to 'tmp://scene.scene'") !=
		  std::string::npos);

	std::filesystem::remove_all(dir);
}

TEST_CASE("pausing a background tab does not mark its document unsaved") {
	Fixture f;
	Scene first = f.scenes.Create("First");
	first.SetSourcePath("tmp://first.scene");
	first.CreateEntity("Child");
	Scene second = f.scenes.Create("Second");
	second.SetSourcePath("tmp://second.scene");
	std::vector<editor::SceneDocumentSaveState> states;
	editor::UpdateSceneDocumentStates(states, {first, second});

	REQUIRE(editor::ActivateSceneDocument(f.scenes, second));
	editor::UpdateSceneDocumentStates(states, {first, second});
	CHECK_FALSE(editor::IsSceneDocumentUnsaved(states, first));
}

TEST_CASE("a scene without a source path is unsaved and cannot be saved") {
	Fixture f;
	Scene scene = f.scenes.Create("Untitled");
	std::vector<editor::SceneDocumentSaveState> states;
	editor::UpdateSceneDocumentStates(states, {scene});
	CHECK(editor::IsSceneDocumentUnsaved(states, scene));

	Log::Clear();
	CHECK_FALSE(editor::SaveSceneDocument(f.fs, f.resources, scene, states));
	REQUIRE_FALSE(Log::GetLines().empty());
	CHECK(Log::GetLines().back().severity == Log::Severity::Error);
}

TEST_CASE("scene dirty state follows undo, redo, save, and a new branch") {
	Fixture f;
	Scene scene = f.scenes.Create("Scene");
	scene.SetSourcePath("tmp://scene.scene");
	std::vector<editor::SceneDocumentSaveState> states;
	editor::UpdateSceneDocumentStates(states, {scene});

	Entity first = editor::ExecuteCreateEntity(
		states, scene, scene.Root(), editor::EntityArchetype::Empty);
	REQUIRE(first.IsAlive());
	CHECK(editor::IsSceneDocumentUnsaved(states, scene));
	REQUIRE(editor::UndoSceneDocument(states, scene));
	CHECK_FALSE(editor::IsSceneDocumentUnsaved(states, scene));
	REQUIRE(editor::RedoSceneDocument(states, scene));
	CHECK(editor::IsSceneDocumentUnsaved(states, scene));

	auto *state = editor::FindSceneDocumentState(states, scene);
	REQUIRE(state != nullptr);
	state->history.MarkSaved();
	CHECK_FALSE(editor::IsSceneDocumentUnsaved(states, scene));
	REQUIRE(editor::UndoSceneDocument(states, scene));
	CHECK(editor::IsSceneDocumentUnsaved(states, scene));
	Entity branch = editor::ExecuteCreateEntity(
		states, scene, scene.Root(), editor::EntityArchetype::Node2D);
	REQUIRE(branch.IsAlive());
	CHECK(editor::IsSceneDocumentUnsaved(states, scene));
	CHECK_FALSE(state->history.CanRedo());
}

TEST_CASE("delete undo revives the parked subtree and incoming references") {
	Fixture f;
	Scene scene = f.scenes.Create("Scene");
	scene.SetSourcePath("tmp://scene.scene");
	Entity parent = scene.CreateEntity("Parent");
	Entity child = parent.CreateChild("Child");
	scene.SetActiveCamera(child);
	std::vector<editor::SceneDocumentSaveState> states;
	editor::UpdateSceneDocumentStates(states, {scene});

	REQUIRE(editor::ExecuteDeleteEntity(states, scene, parent));
	CHECK(parent.IsAlive());
	CHECK_FALSE(parent.IsDescendantOf(scene.Root()));
	CHECK_FALSE(scene.ActiveCamera().IsAlive());
	CHECK(editor::IsSceneDocumentUnsaved(states, scene));

	REQUIRE(editor::UndoSceneDocument(states, scene));
	CHECK(parent.IsDescendantOf(scene.Root()));
	CHECK(child.IsDescendantOf(parent));
	CHECK(scene.ActiveCamera() == child);
	CHECK_FALSE(editor::IsSceneDocumentUnsaved(states, scene));

	REQUIRE(editor::RedoSceneDocument(states, scene));
	CHECK_FALSE(parent.IsDescendantOf(scene.Root()));
	CHECK_FALSE(scene.ActiveCamera().IsAlive());
}

TEST_CASE("command targets survive play-mode snapshot restoration") {
	Fixture f;
	Scene scene = f.scenes.Create("Scene");
	scene.SetSourcePath("tmp://scene.scene");
	REQUIRE(editor::ActivateSceneDocument(f.scenes, scene));
	std::vector<editor::SceneDocumentSaveState> states;
	editor::UpdateSceneDocumentStates(states, {scene});
	Entity created = editor::ExecuteCreateEntity(
		states, scene, scene.Root(), editor::EntityArchetype::Empty);
	REQUIRE(created.IsAlive());
	const Entity oldRoot = scene.Root();

	f.scenes.PlayCurrent();
	f.scenes.Flush();
	REQUIRE(f.scenes.IsPlaying());
	f.scenes.Stop();
	f.scenes.Flush();
	REQUIRE_FALSE(f.scenes.IsPlaying());
	Scene restored = f.scenes.EditorCurrent();
	REQUIRE(restored.IsValid());
	CHECK(restored.Root() != oldRoot);
	editor::UpdateSceneDocumentStates(states, {restored});

	REQUIRE(editor::UndoSceneDocument(states, restored));
	CHECK_FALSE(restored.Root().Find("Entity").IsAlive());
	REQUIRE(editor::RedoSceneDocument(states, restored));
	CHECK(restored.Root().Find("Entity").IsAlive());
}

TEST_CASE("same-path scene tabs keep separate histories after play and stop") {
	Fixture f;
	Scene first = f.scenes.Create("First");
	Scene second = f.scenes.Create("Second");
	first.SetSourcePath("tmp://shared.scene");
	second.SetSourcePath("tmp://shared.scene");
	REQUIRE(editor::ActivateSceneDocument(f.scenes, first));
	std::vector<editor::SceneDocumentSaveState> states;
	editor::UpdateSceneDocumentStates(states, {first, second});
	REQUIRE(editor::ExecuteCreateEntity(
				states, first, first.Root(), editor::EntityArchetype::Empty)
				.IsAlive());
	CHECK(editor::IsSceneDocumentUnsaved(states, first));
	CHECK_FALSE(editor::IsSceneDocumentUnsaved(states, second));

	f.scenes.PlayCurrent();
	f.scenes.Flush();
	f.scenes.Stop();
	f.scenes.Flush();
	const std::vector<Scene> restored = f.scenes.EditorScenes();
	REQUIRE(restored.size() == 2);
	editor::UpdateSceneDocumentStates(states, restored);
	Scene restoredFirst;
	Scene restoredSecond;
	for (Scene scene : restored) {
		if (scene.Name() == "First")
			restoredFirst = scene;
		if (scene.Name() == "Second")
			restoredSecond = scene;
	}
	REQUIRE(restoredFirst.IsValid());
	REQUIRE(restoredSecond.IsValid());
	CHECK(editor::IsSceneDocumentUnsaved(states, restoredFirst));
	CHECK_FALSE(editor::IsSceneDocumentUnsaved(states, restoredSecond));
}

TEST_CASE("reflected field edits are one reversible dirty command") {
	Fixture f;
	Scene scene = f.scenes.Create("Scene");
	scene.SetSourcePath("tmp://scene.scene");
	std::vector<editor::SceneDocumentSaveState> states;
	editor::UpdateSceneDocumentStates(states, {scene});
	Entity node = editor::ExecuteCreateEntity(
		states, scene, scene.Root(), editor::EntityArchetype::Node2D);
	REQUIRE(node.IsAlive());
	auto *state = editor::FindSceneDocumentState(states, scene);
	REQUIRE(state != nullptr);
	state->history.MarkSaved();

	ComponentType transformType = f.world.FindComponent("Transform2D");
	const FieldInfo *position = transformType.FindField("position");
	REQUIRE(position != nullptr);
	editor::FieldValue before;
	REQUIRE(editor::CaptureFieldValue(node, transformType, *position, before));
	node.Get<Transform2D>()->position = Vector2(12.f, 34.f);
	editor::FieldValue after;
	REQUIRE(editor::CaptureFieldValue(node, transformType, *position, after));
	REQUIRE(editor::RecordFieldEdit(states, scene, node, transformType,
									*position, before, after));
	CHECK(editor::IsSceneDocumentUnsaved(states, scene));

	REQUIRE(editor::UndoSceneDocument(states, scene));
	CHECK(node.Get<Transform2D>()->position == Vector2());
	CHECK_FALSE(editor::IsSceneDocumentUnsaved(states, scene));
	REQUIRE(editor::RedoSceneDocument(states, scene));
	CHECK(node.Get<Transform2D>()->position == Vector2(12.f, 34.f));
}

TEST_CASE("a live edit during play is not recorded and Stop restores the authored value") {
	Fixture f;
	Scene scene = f.scenes.Create("Scene");
	std::vector<editor::SceneDocumentSaveState> states;
	editor::UpdateSceneDocumentStates(states, {scene});
	Entity node = editor::ExecuteCreateEntity(
		states, scene, scene.Root(), editor::EntityArchetype::Node2D);
	REQUIRE(node.IsAlive());
	node.Get<Transform2D>()->position = Vector2(5.f, 6.f);
	const std::string path = node.GetPath();
	auto *state = editor::FindSceneDocumentState(states, scene);
	REQUIRE(state != nullptr);
	state->history.MarkSaved();
	REQUIRE(f.scenes.SetCurrent(scene));

	f.scenes.PlayCurrent();
	f.scenes.Flush();
	Entity live = f.scenes.Current().Root().Find(node.GetName());
	REQUIRE(live.IsAlive());
	ComponentType transformType = f.world.FindComponent("Transform2D");
	const FieldInfo *position = transformType.FindField("position");
	REQUIRE(position != nullptr);
	editor::FieldValue before;
	REQUIRE(editor::CaptureFieldValue(live, transformType, *position, before));
	live.Get<Transform2D>()->position = Vector2(90.f, 91.f);
	editor::FieldValue after;
	REQUIRE(editor::CaptureFieldValue(live, transformType, *position, after));
	// The editor edits no document while playing (an invalid scene).
	CHECK_FALSE(editor::RecordFieldEdit(states, Scene(), live, transformType, *position,
		before, after));
	CHECK_FALSE(editor::IsSceneDocumentUnsaved(states, scene));

	f.scenes.Stop();
	f.scenes.Flush();
	const Entity restored = f.world.Find(path);
	REQUIRE(restored.IsAlive());
	CHECK(restored.Get<Transform2D>()->position == Vector2(5.f, 6.f));
}

TEST_CASE("new scene documents append without reordering existing tabs") {
	Fixture f;
	Scene first = f.scenes.Create("First");
	Scene second = f.scenes.Create("Second");
	Scene third = f.scenes.Create("Third");
	std::vector<Entity> order = {first.Root(), second.Root()};

	editor::UpdateSceneDocumentOrder(order, {third, first, second}, third);
	REQUIRE(order.size() == 3);
	CHECK(order[0] == first.Root());
	CHECK(order[1] == second.Root());
	CHECK(order[2] == third.Root());

	Scene fourth = f.scenes.Create("Fourth");
	editor::UpdateSceneDocumentOrder(order, {fourth, third, first}, first);
	REQUIRE(order.size() == 3);
	CHECK(order[0] == first.Root());
	CHECK(order[1] == third.Root());
	CHECK(order[2] == fourth.Root());
}

TEST_CASE("closing the active document selects a neighboring tab") {
	Fixture f;
	Scene first = f.scenes.Create("First");
	Scene second = f.scenes.Create("Second");
	Scene third = f.scenes.Create("Third");
	REQUIRE(editor::ActivateSceneDocument(f.scenes, second));
	const std::vector<Scene> documents = {first, second, third};

	REQUIRE(editor::CloseSceneDocument(f.scenes, second, documents));
	CHECK(f.scenes.Current().Root() == third.Root());
	CHECK_FALSE(first.IsEnabled());
	CHECK(third.IsEnabled());
	CHECK(second.IsValid());

	f.scenes.Flush();
	CHECK_FALSE(second.IsValid());
	CHECK(f.scenes.Loaded().size() == 2);
}

TEST_CASE("closing the last document clears current") {
	Fixture f;
	Scene only = f.scenes.Create("Only");
	REQUIRE(editor::ActivateSceneDocument(f.scenes, only));

	REQUIRE(editor::CloseSceneDocument(f.scenes, only, {only}));
	CHECK_FALSE(f.scenes.Current().IsValid());
	f.scenes.Flush();
	CHECK(f.scenes.Loaded().empty());
}

TEST_CASE("scene document actions are locked during play") {
	Fixture f;
	Scene scene = f.scenes.Create("Scene");
	REQUIRE(editor::ActivateSceneDocument(f.scenes, scene));
	f.scenes.PlayCurrent();
	f.scenes.Flush();
	REQUIRE(f.scenes.IsPlaying());

	CHECK_FALSE(editor::ActivateSceneDocument(f.scenes, scene));
	CHECK_FALSE(editor::CloseSceneDocument(f.scenes, scene, {scene}));
}

TEST_CASE("Scene tabs retain source navigation across deferred ImGui selection") {
	Fixture f;
	Scene first = f.scenes.Create("First");
	Scene second = f.scenes.Create("Second");
	first.Root().Get<SceneRoot>()->sourcePath = "project://first.scene";
	second.Root().Get<SceneRoot>()->sourcePath = "project://second.scene";
	std::vector<Scene> documents{first, second};
	std::vector<editor::SceneDocumentSaveState> states;
	Entity visibleRoot;
	REQUIRE(editor::ActivateSceneDocument(f.scenes, first));

	ImGui::CreateContext();

	struct Cleanup {
		~Cleanup() {
			ImGui::DestroyContext();
		}
	} cleanup;

	auto &io = ImGui::GetIO();
	io.IniFilename = nullptr;
	io.DisplaySize = {640.f, 480.f};
	io.DeltaTime = 1.f / 60.f;
	unsigned char *pixels;
	int width, height;
	io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

	ImVec2 firstTabCenter;
	const auto frame = [&] {
		ImGui::NewFrame();
		ImGui::SetNextWindowPos({0.f, 0.f});
		ImGui::SetNextWindowSize({600.f, 400.f});
		ImGui::Begin("Scene tabs test");
		const auto actions = editor::DrawSceneDocumentTabs(documents, f.scenes.EditorCurrent(),
			visibleRoot, states, f.scenes.IsPlaying());
		if (actions.activate.IsValid()) {
			REQUIRE(editor::ActivateSceneDocument(f.scenes, actions.activate));
		}
		CHECK_FALSE(actions.close.IsValid());
		const auto *bar =
			ImGui::GetCurrentContext()->TabBars.GetByKey(ImGui::GetID("##scene_documents"));
		REQUIRE(bar != nullptr);
		const auto &tab = bar->Tabs[0];
		firstTabCenter = {bar->BarRect.Min.x + tab.Offset + tab.Width / 2.f,
			(bar->BarRect.Min.y + bar->BarRect.Max.y) / 2.f};
		ImGui::End();
		ImGui::Render();
	};
	const auto settlesOn = [&](Scene expected) {
		for (int i = 0; i < 4; ++i) {
			frame();
			CHECK(f.scenes.EditorCurrent().Root() == expected.Root());
		}
		CHECK(visibleRoot == expected.Root());
		CHECK(expected.IsEnabled());
	};
	settlesOn(first);

	// Both directions matter: the old tab may be submitted before or after the target.
	for (Scene target : {second, first, second}) {
		REQUIRE(editor::TryOpenAsset(f.scenes, target.SourcePath(), editor::AssetKind::Scene));
		settlesOn(target);
		CHECK(f.scenes.EditorScenes().size() == 2);
	}

	// A real tab click must still activate a different document.
	io.AddMousePosEvent(firstTabCenter.x, firstTabCenter.y);
	frame();
	io.AddMouseButtonEvent(ImGuiMouseButton_Left, true);
	frame();
	io.AddMouseButtonEvent(ImGuiMouseButton_Left, false);
	frame();
	settlesOn(first);

	// Navigation to a newly opened tab uses the same explicit selection path.
	Scene third = f.scenes.Create("Third");
	documents.push_back(third);
	REQUIRE(editor::ActivateSceneDocument(f.scenes, third));
	settlesOn(third);

	// A second request can arrive before ImGui has displayed the first one.
	REQUIRE(editor::ActivateSceneDocument(f.scenes, second));
	frame();
	REQUIRE(editor::ActivateSceneDocument(f.scenes, first));
	settlesOn(first);

	// Closing the active tab keeps the successor selected once ImGui removes the old tab.
	REQUIRE(editor::CloseSceneDocument(f.scenes, first, documents));
	f.scenes.Flush();
	documents = {second, third};
	settlesOn(second);

	f.scenes.PlayCurrent();
	f.scenes.Flush();
	REQUIRE(f.scenes.IsPlaying());
	for (int i = 0; i < 3; ++i) {
		frame();
		CHECK(f.scenes.EditorCurrent().Root() == second.Root());
		CHECK(visibleRoot == second.Root());
	}
}
