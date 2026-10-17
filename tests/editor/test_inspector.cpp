#include <doctest/doctest.h>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "core/filesystem/file_system.hpp"
#include "core/project_settings.hpp"
#include "core/reflection/field_type.hpp"
#include "ecs/entity.hpp"
#include "ecs/world.hpp"
#include "editor/inspector.hpp"
#include "editor/scene_commands.hpp"
#include "resource/resource_registry.hpp"
#include "scene/scene.hpp"
#include "scene/scene_manager.hpp"
#include "script/script_server.hpp"

using namespace sowa;

namespace {

struct Props {
	f32 shown = 1.f;
	f32 derived = 2.f;
	f32 secret = 3.f;
	f32 savedOnly = 4.f;
};

std::vector<std::string> FieldNames(const std::vector<const FieldInfo *> &fields) {
	std::vector<std::string> names;
	names.reserve(fields.size());
	for (const FieldInfo *field : fields)
		names.push_back(field->name);
	return names;
}

std::filesystem::path makeScriptTempDir() {
	const auto dir = std::filesystem::temp_directory_path() /
					 "sowa_script_inspector_test";
	std::filesystem::remove_all(dir);
	std::filesystem::create_directories(dir);
	return dir;
}

void writeScriptFile(const std::filesystem::path &path, std::string_view contents) {
	std::filesystem::create_directories(path.parent_path());
	std::ofstream out(path, std::ios::binary);
	out.write(contents.data(), static_cast<std::streamsize>(contents.size()));
}

} // namespace

TEST_CASE("DisplayName splits camelCase and sentence-cases the words") {
	CHECK(editor::DisplayName("degreesPerSecond") == "Degrees per second");
	CHECK(editor::DisplayName("maxDistance") == "Max distance");
	CHECK(editor::DisplayName("zIndex") == "Z index");
	CHECK(editor::DisplayName("zIndexMode") == "Z index mode");
	CHECK(editor::DisplayName("position") == "Position");
	CHECK(editor::DisplayName("Name") == "Name");
	CHECK(editor::DisplayName("activeCamera") == "Active camera");
	CHECK(editor::DisplayName("") == "");
}

TEST_CASE("ExposedFields keeps Expose and drops internal and serialize-only") {
	World world;
	ComponentType type = world.RegisterComponent<Props>("Props")
							 .Field("shown", &Props::shown)
							 .Field("derived", &Props::derived, FieldFlags::Expose)
							 .Field("secret", &Props::secret, FieldFlags::None)
							 .Field("savedOnly", &Props::savedOnly, FieldFlags::Serialize)
							 .Type();

	const std::vector<std::string> names = FieldNames(editor::ExposedFields(type));
	REQUIRE(names.size() == 2);
	CHECK(names[0] == "shown");
	CHECK(names[1] == "derived");
}

TEST_CASE("CollectFields walks the entity's exposed fields") {
	World world;
	ComponentType type = world.RegisterComponent<Props>("Props")
							 .Field("shown", &Props::shown)
							 .Field("secret", &Props::secret, FieldFlags::None)
							 .Type();

	Entity entity = world.CreateEntity();
	entity.Add(Props{10.f, 0.f, 99.f, 0.f});

	const std::vector<editor::InspectedField> fields = editor::CollectFields(entity);
	REQUIRE(fields.size() == 1);
	CHECK(fields[0].component == type);
	REQUIRE(fields[0].field != nullptr);
	CHECK(fields[0].field->name == "shown");

	FieldRef ref = entity.GetField(type, "shown");
	REQUIRE(ref);
	f32 value = 0.f;
	REQUIRE(ref.Get(value));
	CHECK(value == doctest::Approx(10.f));
	REQUIRE(ref.Set(42.f));
	REQUIRE(entity.Get<Props>() != nullptr);
	CHECK(entity.Get<Props>()->shown == doctest::Approx(42.f));
}

TEST_CASE("CollectFields is empty on a dead entity") {
	CHECK(editor::CollectFields(Entity()).empty());
}

TEST_CASE("CanEdit is false when dead, playing, or instance-locked") {
	World world;
	Scene scene = CreateScene(world, "Level");
	Entity entity = scene.CreateEntity("Mob");
	CHECK(editor::CanEdit(entity, false));
	CHECK_FALSE(editor::CanEdit(entity, true));
	CHECK_FALSE(editor::CanEdit(Entity(), false));

	Entity instance = scene.CreateEntity("Inst");
	instance.Add(SceneInstance{});
	Entity locked = instance.CreateChild("Child");
	CHECK(IsInstanceLocked(locked));
	CHECK_FALSE(editor::CanEdit(locked, false));
	CHECK(editor::CanEdit(instance, false));
}

TEST_CASE("CanEditFields allows live entities during play but not frozen documents") {
	World world;
	ResourceRegistry resources;
	FileSystem fs;
	SceneManager scenes(world, resources, fs);
	Scene level = scenes.Create("Level");
	Entity mob = level.CreateEntity("Mob");
	Entity instance = level.CreateEntity("Inst");
	instance.Add(SceneInstance{});
	Entity locked = instance.CreateChild("Child");
	REQUIRE(scenes.SetCurrent(level));

	// Stopped: the same rule as CanEdit.
	CHECK(editor::CanEditFields(mob, scenes));
	CHECK_FALSE(editor::CanEditFields(locked, scenes));
	CHECK_FALSE(editor::CanEditFields(Entity(), scenes));

	scenes.PlayCurrent();
	scenes.Flush();
	REQUIRE(scenes.IsPlaying());
	const Entity live = scenes.Current().Root().Find("Mob");
	const Entity liveLocked = scenes.Current().Root().Find("Inst/Child");
	REQUIRE(live.IsAlive());
	REQUIRE(liveLocked.IsAlive());
	CHECK(editor::CanEditFields(live, scenes));
	CHECK(editor::CanEditFields(liveLocked, scenes));
	CHECK_FALSE(editor::CanEditFields(mob, scenes));
	// Structure stays locked during play.
	CHECK_FALSE(editor::CanEdit(live, true));
}

TEST_CASE("inspector script actions load attach register and remove Lua components") {
	const auto root = makeScriptTempDir();
	writeScriptFile(root / "spin.lua",
					"world:component('Spin', { speed = 2.5 })\n");
	writeScriptFile(root / "plain.lua", "return true\n");

	FileSystem fs;
	fs.MountFolder("res", root);
	World world;
	ScriptServer scripts(world);
	ProjectSettings project;
	Entity entity = world.CreateEntity();

	const editor::ScriptEditResult attached = editor::TryAttachEntityScript(
		scripts, fs, project, entity, "res:///spin.lua", false);
	REQUIRE(attached);
	CHECK(attached.path == "res://spin.lua");
	ComponentType spin = world.FindComponent("Spin");
	REQUIRE(spin.IsValid());
	CHECK(entity.Has(spin));
	f32 speed = 0.f;
	REQUIRE(entity.GetField(spin, "speed").Get(speed));
	CHECK(speed == doctest::Approx(2.5f));
	CHECK(project.application.scripts ==
		  std::vector<std::string>{"res://spin.lua"});

	CHECK(editor::TryAttachEntityScript(scripts, fs, project, entity,
										"res://spin.lua", false));
	CHECK(project.application.scripts.size() == 1);
	CHECK(editor::TryRemoveEntityScript(scripts, entity, "res://spin.lua", false));
	CHECK_FALSE(entity.Has(spin));

	const editor::ScriptEditResult noComponent = editor::TryAttachEntityScript(
		scripts, fs, project, entity, "res://plain.lua", false);
	CHECK_FALSE(noComponent);
	CHECK(noComponent.error.find("does not define") != std::string::npos);
	CHECK_FALSE(editor::TryAttachEntityScript(scripts, fs, project, entity,
											  "res://spin.lua", true));

	std::filesystem::remove_all(root);
}

TEST_CASE("undoing script removal restores its edited fields") {
	const auto root = makeScriptTempDir();
	writeScriptFile(root / "spin.lua",
					"world:component('Spin', { speed = 2.5 })\n");

	FileSystem fs;
	fs.MountFolder("res", root);
	World world;
	ResourceRegistry resources;
	SceneManager scenes(world, resources, fs);
	ScriptServer scripts(world);
	ProjectSettings project;
	Scene scene = scenes.Create("Scene");
	scene.SetSourcePath("res://scene.scene");
	Entity entity = scene.CreateEntity("Spinner");
	std::vector<editor::SceneDocumentSaveState> states;
	editor::UpdateSceneDocumentStates(states, {scene});

	REQUIRE(editor::TryAttachEntityScript(
		scripts, fs, project, entity, "res://spin.lua", false));
	ComponentType spin = world.FindComponent("Spin");
	REQUIRE(spin.IsValid());
	REQUIRE(entity.GetField(spin, "speed").Set(9.25f));
	REQUIRE(editor::ExecuteScriptRemove(
		states, scene, entity, scripts, "res://spin.lua"));
	CHECK_FALSE(entity.Has(spin));

	REQUIRE(editor::UndoSceneDocument(states, scene));
	REQUIRE(entity.Has(spin));
	f32 restored = 0.f;
	REQUIRE(entity.GetField(spin, "speed").Get(restored));
	CHECK(restored == doctest::Approx(9.25f));
	REQUIRE(editor::RedoSceneDocument(states, scene));
	CHECK_FALSE(entity.Has(spin));

	std::filesystem::remove_all(root);
}
