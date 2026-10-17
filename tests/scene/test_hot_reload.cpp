#include <chrono>
#include <filesystem>
#include <fstream>
#include <functional>
#include <thread>

#include <doctest/doctest.h>

#include "core/filesystem/file_system.hpp"
#include "ecs/components/transform2d.hpp"
#include "ecs/world.hpp"
#include "editor/scene_documents.hpp"
#include "resource/image_texture.hpp"
#include "resource/resource_registry.hpp"
#include "scene/packed_scene.hpp"
#include "scene/scene_manager.hpp"
#include "scene/scene_serializer.hpp"
#include "script/script_server.hpp"

using namespace sowa;

namespace {
struct Fixture {
	std::filesystem::path dir =
		std::filesystem::temp_directory_path() /
		("sowa_hot_reload_" +
			std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
	FileSystem fs;
	ResourceRegistry resources;
	World world;
	SceneManager scenes{world, resources, fs};

	Fixture() {
		std::filesystem::remove_all(dir);
		std::filesystem::create_directories(dir);
		fs.MountFolder("tmp", dir);
		resources.SetContext({nullptr, &fs});
		resources.RegisterType<PackedScene>("PackedScene").Field("path", &PackedScene::path);
		resources.RegisterType<ImageTexture>("ImageTexture").Field("path", &ImageTexture::path);
	}

	~Fixture() {
		fs.EnableHotReload(false);
		resources.Clear();
		std::filesystem::remove_all(dir);
	}

	void write(const std::string &name, const std::string &text) {
		std::ofstream(dir / name, std::ios::binary) << text;
	}

	void change(const std::string &name) {
		fs.Events()->changed("tmp://" + name);
		scenes.Flush();
	}

	Scene open(const std::string &name) {
		scenes.Open("tmp://" + name);
		scenes.Flush();
		return scenes.FindByPath("tmp://" + name);
	}

	Ref<PackedScene> packed(const std::string &name) {
		auto packed = resources.Create<PackedScene>();
		packed->path = "tmp://" + name;
		resources.Reload(packed->Id());
		return packed;
	}
};

std::string sceneText(int rotation, const std::string &child = "Child") {
	return "root:\n  name: Source\n  components:\n    SceneRoot: {}\n"
		   "    Transform2D: {position: [0, 0], rotation: " +
		   std::to_string(rotation) + ", scale: [1, 1]}\n  children:\n    - name: " + child +
		   "\n      components:\n        Transform2D: {}\n";
}

void tga(const std::filesystem::path &path, unsigned char width) {
	std::string bytes(18, '\0');
	bytes[2] = 2;
	bytes[12] = static_cast<char>(width);
	bytes[14] = 1;
	bytes[16] = 24;
	bytes.append(width * 3, '\xff');
	std::ofstream file(path, std::ios::binary);
	file.write(bytes.data(), bytes.size());
}

void pump(FileSystem &fs, std::chrono::milliseconds duration) {
	const auto end = std::chrono::steady_clock::now() + duration;
	do {
		fs.DispatchChanges();
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	} while (std::chrono::steady_clock::now() < end);
}

bool waitFor(FileSystem &fs, const std::function<bool()> &ready) {
	// Allow native subscription recovery (backoff is capped at five seconds).
	const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
	while (!ready()) {
		if (std::chrono::steady_clock::now() >= deadline) {
			return false;
		}
		pump(fs, std::chrono::milliseconds(20));
	}
	return true;
}
} // namespace

#ifndef __EMSCRIPTEN__
TEST_CASE(
	"Native hot reload updates every texture subscriber after writes and atomic replacement") {
	Fixture f;
	f.fs.EnableHotReload();
	tga(f.dir / "texture.tga", 1);
	auto a = f.resources.Create<ImageTexture>();
	auto b = f.resources.Create<ImageTexture>();
	a->path = "tmp://texture.tga";
	b->path = "tmp://./texture.tga";
	f.resources.Reload(a->Id());
	f.resources.Reload(b->Id());
	REQUIRE(a->Width() == 1);
	int reloads = 0;
	f.resources.Reloaded().append([&](ResourceID) { ++reloads; });
	pump(f.fs, std::chrono::milliseconds(200));
	reloads = 0;
	tga(f.dir / "texture.tga", 2);
	REQUIRE(waitFor(f.fs, [&] { return a->Width() == 2; }));
	CHECK(a->Width() == 2);
	CHECK(b->Width() == 2);
	CHECK(reloads == 2);
	tga(f.dir / "replacement.tga", 3);
	std::filesystem::rename(f.dir / "replacement.tga", f.dir / "texture.tga");
	REQUIRE(waitFor(f.fs, [&] { return a->Width() == 3; }));
	CHECK(a->Width() == 3);
	CHECK(b->Width() == 3);
	const int before = reloads;
	pump(f.fs, std::chrono::milliseconds(200));
	CHECK(reloads == before);
	// A missing source can be recreated; the parent subscription survives.
	std::filesystem::remove(f.dir / "texture.tga");
	pump(f.fs, std::chrono::milliseconds(200));
	tga(f.dir / "texture.tga", 4);
	REQUIRE(waitFor(f.fs, [&] { return a->Width() == 4; }));
	CHECK(a->Width() == 4);
	CHECK(b->Width() == 4);
}
#endif

TEST_CASE("Hot reload dependencies follow resource path edits and removal") {
	Fixture f;
	tga(f.dir / "a.tga", 1);
	tga(f.dir / "b.tga", 2);
	auto texture = f.resources.Create<ImageTexture>();
	texture->path = "tmp://a.tga";
	f.resources.Reload(texture->Id());
	texture->path = "tmp://b.tga";
	f.resources.Reload(texture->Id());
	int reloads = 0;
	f.resources.Reloaded().append([&](ResourceID) { ++reloads; });
	f.change("a.tga");
	CHECK(reloads == 0);
	f.change("b.tga");
	CHECK(reloads == 1);
	f.resources.Remove(texture->Id());
	f.change("b.tga");
	CHECK(reloads == 1);
}

TEST_CASE(
	"Scene hot reload updates all instances including inactive tabs and preserves root overrides") {
	Fixture f;
	f.write("source.scene", sceneText(10));
	auto packed = f.packed("source.scene");
	Scene a = f.scenes.Create("HostA"), b = f.scenes.Create("HostB");
	a.Own(f.resources, packed->Id());
	b.Own(f.resources, packed->Id());
	Entity first = packed->Instantiate(f.world, f.resources, a.Root());
	Entity second = packed->Instantiate(f.world, f.resources, b.Root());
	first.SetName("Custom");
	first.Get<Transform2D>()->position = {12, 34};
	b.SetEnabled(false);
	Entity oldChild = first.Find("Child");
	f.write("source.scene", sceneText(45, "NewChild"));
	f.change("source.scene");
	CHECK(first.IsAlive());
	CHECK(first.GetName() == "Custom");
	CHECK(first.Get<Transform2D>()->position == Vector2(12, 34));
	CHECK(first.Get<Transform2D>()->rotation == doctest::Approx(45));
	CHECK(second.Get<Transform2D>()->rotation == doctest::Approx(45));
	CHECK_FALSE(oldChild.IsAlive());
	CHECK(first.Find("NewChild").IsAlive());
	CHECK(second.Find("NewChild").IsAlive());
	CHECK_FALSE(second.Find("NewChild").IsEnabled());
	CHECK(f.resources.OwnerCount(packed->Id()) == 2);
	f.write("source.scene", sceneText(90, "Third"));
	f.change("source.scene");
	CHECK(first.Get<Transform2D>()->rotation == doctest::Approx(90));
	CHECK(first.Get<Transform2D>()->position == Vector2(12, 34));
}

TEST_CASE("Scene hot reload retains last valid instances after malformed saves") {
	Fixture f;
	f.write("source.scene", sceneText(10));
	auto packed = f.packed("source.scene");
	Scene host = f.scenes.Create("Host");
	host.Own(f.resources, packed->Id());
	Entity instance = packed->Instantiate(f.world, f.resources, host.Root());
	Entity child = instance.Find("Child");
	f.write("source.scene", "root: [broken");
	f.change("source.scene");
	CHECK(packed->IsLoaded());
	CHECK(child.IsAlive());
	CHECK(instance.Find("Child") == child);
	f.write("source.scene", "root:\n  components: {SceneRoot: {}}\n  children: invalid\n");
	f.change("source.scene");
	CHECK(child.IsAlive());
	f.write("source.scene", sceneText(22, "Fixed"));
	f.change("source.scene");
	CHECK(instance.Find("Fixed").IsAlive());
}

TEST_CASE(
	"Open scene hot reload preserves document handles and invalidates editor history indexes") {
	Fixture f;
	f.write("source.scene", sceneText(10));
	Scene scene = f.open("source.scene");
	REQUIRE(scene.IsValid());
	f.scenes.SetCurrent(scene);
	std::vector<editor::SceneDocumentSaveState> states;
	editor::UpdateSceneDocumentStates(states, f.scenes.EditorScenes());
	Entity child = scene.Root().Find("Child");
	f.write("source.scene", sceneText(70, "Replacement"));
	f.change("source.scene");
	CHECK(f.scenes.Current().Root() == scene.Root());
	CHECK_FALSE(child.IsAlive());
	CHECK(scene.Root().Find("Replacement").IsAlive());
	editor::UpdateSceneDocumentStates(states, f.scenes.EditorScenes());
	CHECK(states[0].context->reloadRevision == 1);
	CHECK_FALSE(states[0].history.IsDirty());
	CHECK(states[0].context->entityById.size() == 2);
}

TEST_CASE("Editor save notifications refresh instances without rebuilding the saved document") {
	Fixture f;
	f.write("source.scene", sceneText(10));
	Scene document = f.open("source.scene");
	auto packed = f.packed("source.scene");
	Scene host = f.scenes.Create("Host");
	host.Own(f.resources, packed->Id());
	Entity instance = packed->Instantiate(f.world, f.resources, host.Root());
	Entity child = document.Root().Find("Child");
	document.Root().Get<Transform2D>()->rotation = 80;
	REQUIRE(SaveSceneToFile(f.fs, "tmp://source.scene", document, f.resources) == FsError::Ok);
	f.change("source.scene");
	CHECK(child.IsAlive());
	CHECK(instance.Get<Transform2D>()->rotation == doctest::Approx(80));
}

TEST_CASE("Dirty scene guard protects documents while their instances update") {
	Fixture f;
	f.write("source.scene", sceneText(10));
	Scene document = f.open("source.scene");
	f.scenes.SetReloadGuard([](Scene) { return false; });
	f.write("source.scene", sceneText(70));
	f.change("source.scene");
	CHECK(document.Root().Get<Transform2D>()->rotation == doctest::Approx(10));
}

TEST_CASE("Scene updates received during play apply after stop restores editor documents") {
	Fixture f;
	f.write("source.scene", sceneText(10));
	Scene document = f.open("source.scene");
	f.scenes.SetCurrent(document);
	f.scenes.PlayCurrent();
	f.scenes.Flush();
	REQUIRE(f.scenes.IsPlaying());
	f.write("source.scene", sceneText(70));
	f.change("source.scene");
	CHECK(document.Root().Get<Transform2D>()->rotation == doctest::Approx(10));
	f.scenes.Stop();
	f.scenes.Flush();
	CHECK(f.scenes.Current().Root().Get<Transform2D>()->rotation == doctest::Approx(70));
}

TEST_CASE("Nested scene instances update and host entity references follow replaced children") {
	Fixture f;

	struct Target {
		Entity entity;
	};

	f.world.RegisterComponent<Target>("Target").Field("entity", &Target::entity);
	f.write("inner.scene", sceneText(10));
	f.write("outer.scene",
		"resources:\n  - id: 101\n    type: PackedScene\n    data: {path: 'tmp://inner.scene'}\n"
		"root:\n  name: Outer\n  components: {SceneRoot: {}}\n  children:\n    - instance: 101\n   "
		"   name: Nested\n");
	auto packed = f.packed("outer.scene");
	Scene host = f.scenes.Create("Host");
	host.Own(f.resources, packed->Id());
	Entity instance = packed->Instantiate(f.world, f.resources, host.Root());
	Entity nested = instance.Find("Nested");
	REQUIRE(nested.IsAlive());
	Entity oldChild = nested.Find("Child");
	REQUIRE(oldChild.IsAlive());
	Entity holder = host.CreateEntity("Holder");
	holder.Add(Target{oldChild});
	f.write("inner.scene", sceneText(80));
	f.change("inner.scene");
	CHECK(nested.IsAlive());
	CHECK_FALSE(oldChild.IsAlive());
	CHECK(nested.Get<Transform2D>()->rotation == doctest::Approx(80));
	CHECK(holder.Get<Target>()->entity == nested.Find("Child"));
	CHECK(f.resources.OwnerCount(ResourceID{101}) == 1);
}

TEST_CASE("Scene hot reload updates shared resource definitions without replacing resource IDs") {
	Fixture f;
	tga(f.dir / "a.tga", 1);
	tga(f.dir / "b.tga", 2);
	auto source = [](const char *file) {
		return std::string(
				   "resources:\n  - id: 100\n    type: ImageTexture\n    data: {path: 'tmp://") +
			   file + "'}\n" + sceneText(10);
	};
	f.write("source.scene", source("a.tga"));
	Scene document = f.open("source.scene");
	auto packed = f.packed("source.scene");
	Scene host = f.scenes.Create("Host");
	host.Own(f.resources, packed->Id());
	packed->Instantiate(f.world, f.resources, host.Root());
	auto texture = f.resources.Get<ImageTexture>(ResourceID{100});
	REQUIRE(texture);
	CHECK(texture->Width() == 1);
	CHECK(f.resources.OwnerCount(texture->Id()) == 2);
	f.write("source.scene", source("b.tga"));
	f.change("source.scene");
	CHECK(f.resources.Get(texture->Id()) == texture);
	CHECK(texture->Width() == 2);
	CHECK(f.resources.OwnerCount(texture->Id()) == 2);
	f.write("source.scene", sceneText(10));
	f.change("source.scene");
	CHECK_FALSE(f.resources.Has(ResourceID{100}));
}

#ifndef __EMSCRIPTEN__
TEST_CASE("Native notifications reload a newly saved scene and its editor instances") {
	Fixture f;
	f.fs.EnableHotReload();
	Scene document = f.scenes.Create("Source");
	document.Root().Add<Transform2D>();
	document.SetSourcePath("tmp://source.scene");
	REQUIRE(SaveSceneToFile(f.fs, document.SourcePath(), document, f.resources) == FsError::Ok);
	// No Load has occurred: Save must establish the native subscription.
	f.write("source.scene", sceneText(30));
	REQUIRE(waitFor(f.fs, [&] {
		f.scenes.Flush();
		return document.Root().Get<Transform2D>()->rotation == 30;
	}));
	REQUIRE(document.Root().Get<Transform2D>()->rotation == doctest::Approx(30));
	auto packed = f.packed("source.scene");
	Scene host = f.scenes.Create("Host");
	host.Own(f.resources, packed->Id());
	Entity instance = packed->Instantiate(f.world, f.resources, host.Root());
	f.write("source.scene", sceneText(60, "NativeChild"));
	REQUIRE(waitFor(f.fs, [&] {
		f.scenes.Flush();
		return instance.Find("NativeChild").IsAlive();
	}));
	CHECK(document.Root().Get<Transform2D>()->rotation == doctest::Approx(60));
	CHECK(instance.Get<Transform2D>()->rotation == doctest::Approx(60));
	CHECK(instance.Find("NativeChild").IsAlive());
}
#endif

TEST_CASE("Resources subscribe to all files read during Reload and disconnect on destruction") {
	Fixture f;

	struct Composite : Resource {
		int reloads = 0;

		void Reload(const ResourceContext &context) override {
			++reloads;
			context.fs->Load("tmp://first.txt");
			context.fs->Load("tmp://second.txt");
		}
	};

	f.write("first.txt", "one");
	f.write("second.txt", "two");
	Ref<Composite> composite;
	{
		ResourceRegistry registry;
		registry.SetContext({nullptr, &f.fs});
		registry.RegisterType<Composite>("Composite");
		composite = registry.Create<Composite>();
		registry.Reload(composite->Id());
		f.change("first.txt");
		f.change("second.txt");
		CHECK(composite->reloads == 3);
	}
	f.change("first.txt");
	CHECK(composite->reloads == 3);
}

TEST_CASE("Scene hot reload preserves components added only on the host instance") {
	Fixture f;

	struct MouseFollower {
		f32 speed = 900;
	};

	f.world.RegisterComponent<MouseFollower>("MouseFollower").Field("speed", &MouseFollower::speed);
	f.write("object.scene", sceneText(10));
	auto packed = f.packed("object.scene");
	Scene demo = f.scenes.Create("Demo");
	demo.Own(f.resources, packed->Id());
	Entity instance = packed->Instantiate(f.world, f.resources, demo.Root());
	instance.Add(MouseFollower{777});
	Scene document = f.open("object.scene");
	REQUIRE(editor::ActivateSceneDocument(f.scenes, document));
	document.Root().Find("Child").Get<Transform2D>()->position.x = 123;
	REQUIRE(SaveSceneToFile(f.fs, document.SourcePath(), document, f.resources) == FsError::Ok);
	CHECK_NOTHROW(f.change("object.scene"));
	REQUIRE(instance.Has<MouseFollower>());
	CHECK(instance.Get<MouseFollower>()->speed == doctest::Approx(777));
	CHECK(instance.Find("Child").Get<Transform2D>()->position.x == doctest::Approx(123));
}

#ifndef __EMSCRIPTEN__
TEST_CASE(
	"Native script file notifications update required code and running systems without restart") {
	Fixture f;
	f.fs.EnableHotReload();
	ScriptServer scripts(f.world);
	f.write("helper.lua", "return 1\n");
	f.write("main.lua",
		"local step = require('tmp://helper.lua'); world:component('ScriptCounter', {n = 0}); "
		"world:system('HotTick'):with('ScriptCounter'):each(function(e, c) c.n = c.n + step "
		"end)\n");
	REQUIRE(scripts.LoadModules(f.fs, {"tmp://main.lua"}));
	Entity entity = f.world.CreateEntity();
	REQUIRE(scripts.AttachScript(entity, "tmp://main.lua"));
	f.world.Progress(0.1f);
	auto value = [&] {
		return *entity.GetField(f.world.FindComponent("ScriptCounter"), "n").As<f32>();
	};
	REQUIRE(value() == doctest::Approx(1));
	f.write("replacement.lua", "return 10\n");
	std::filesystem::rename(f.dir / "replacement.lua", f.dir / "helper.lua");
	REQUIRE(waitFor(f.fs, [&] {
		REQUIRE(scripts.FlushFileChanges());
		const auto before = value();
		f.world.Progress(0.1f);
		return value() - before == 10;
	}));
}
#endif
