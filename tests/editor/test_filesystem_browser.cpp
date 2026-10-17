#include <doctest/doctest.h>

#include <filesystem>
#include <fstream>

#include "core/filesystem/file_system.hpp"
#include "ecs/components/transform2d.hpp"
#include "ecs/world.hpp"
#include "editor/filesystem_browser.hpp"
#include "resource/resource_registry.hpp"
#include "scene/scene_manager.hpp"
#include "scene/scene_serializer.hpp"
#include "script/script_server.hpp"

using namespace sowa;

namespace {

std::filesystem::path makeTempDir() {
	const auto dir = std::filesystem::temp_directory_path() / "sowa_fs_browser_test";
	std::filesystem::remove_all(dir);
	std::filesystem::create_directories(dir);
	return dir;
}

void writeFile(const std::filesystem::path &path, std::string_view contents) {
	std::filesystem::create_directories(path.parent_path());
	std::ofstream out(path, std::ios::binary);
	out.write(contents.data(), static_cast<std::streamsize>(contents.size()));
}

} // namespace

TEST_CASE("KindFromName maps extensions and directories") {
	CHECK(editor::KindFromName("sprites", true) == editor::AssetKind::Directory);
	CHECK(editor::KindFromName("level.scene", false) == editor::AssetKind::Scene);
	CHECK(editor::KindFromName("Level.SCENE", false) == editor::AssetKind::Scene);
	CHECK(editor::KindFromName("main.lua", false) == editor::AssetKind::Script);
	CHECK(editor::KindFromName("hero.png", false) == editor::AssetKind::Image);
	CHECK(editor::KindFromName("shot.wav", false) == editor::AssetKind::Audio);
	CHECK(editor::KindFromName("ui.ttf", false) == editor::AssetKind::Font);
	CHECK(editor::KindFromName("readme.txt", false) == editor::AssetKind::Other);
}

TEST_CASE("PrimaryAction opens scenes and nothing else") {
	CHECK(editor::PrimaryAction(editor::AssetKind::Scene) ==
		  editor::AssetAction::OpenScene);
	CHECK(editor::PrimaryAction(editor::AssetKind::Directory) ==
		  editor::AssetAction::None);
	CHECK(editor::PrimaryAction(editor::AssetKind::Image) ==
		  editor::AssetAction::None);
}

TEST_CASE("JoinVirtualPath keeps the scheme slash rules") {
	CHECK(editor::JoinVirtualPath("res://", "scenes") == "res://scenes");
	CHECK(editor::JoinVirtualPath("res://scenes", "demo.scene") ==
		  "res://scenes/demo.scene");
	CHECK(editor::JoinVirtualPath("res://scenes/", "demo.scene") ==
		  "res://scenes/demo.scene");
}

TEST_CASE("Asset path helpers keep mutations inside one directory") {
	CHECK(editor::AssetName("res://scenes/demo.scene") == "demo.scene");
	CHECK(editor::AssetName("res://") == "");
	CHECK(editor::ParentVirtualPath("res://scenes/demo.scene") ==
		  "res://scenes");
	CHECK(editor::ParentVirtualPath("res://demo.scene") == "res://");
	CHECK(editor::IsValidAssetName("New Level.scene"));
	CHECK_FALSE(editor::IsValidAssetName(""));
	CHECK_FALSE(editor::IsValidAssetName(".hidden"));
	CHECK_FALSE(editor::IsValidAssetName("../escape"));
	CHECK_FALSE(editor::IsValidAssetName("nested/file"));
	CHECK_FALSE(editor::IsValidAssetName("trailing."));
}

TEST_CASE("Asset CRUD returns paths and protects the virtual root") {
	const auto root = makeTempDir();
	FileSystem fs;
	fs.MountFolder("res", root);

	const auto folder = editor::CreateAsset(
		fs, "res://", "scenes", editor::AssetCreateKind::Directory);
	REQUIRE(folder);
	CHECK(folder.value == "res://scenes");
	const auto file = editor::CreateAsset(fs, folder.value, "level.scene",
										  editor::AssetCreateKind::File);
	REQUIRE(file);
	CHECK(file.value == "res://scenes/level.scene");
	CHECK(editor::CreateAsset(fs, folder.value, "level.scene",
							  editor::AssetCreateKind::File)
			  .error == FsError::AlreadyExists);

	const auto renamed =
		editor::RenameAsset(fs, file.value, "main.scene");
	REQUIRE(renamed);
	CHECK(renamed.value == "res://scenes/main.scene");
	CHECK(std::filesystem::exists(root / "scenes" / "main.scene"));
	REQUIRE(editor::CreateAsset(fs, folder.value, "existing.scene",
								editor::AssetCreateKind::File));
	CHECK(editor::RenameAsset(fs, renamed.value, "existing.scene").error ==
		  FsError::AlreadyExists);
	CHECK(std::filesystem::exists(root / "scenes" / "main.scene"));
	CHECK(editor::RenameAsset(fs, "res://", "project").error ==
		  FsError::InvalidPath);
	CHECK(editor::DeleteAsset(fs, "res://") == FsError::InvalidPath);

	writeFile(root / "scenes" / "nested" / "inside.txt", "inside");
	CHECK(editor::DeleteAsset(fs, folder.value) == FsError::Ok);
	CHECK_FALSE(std::filesystem::exists(root / "scenes"));
	CHECK(std::filesystem::exists(root));

	std::filesystem::remove_all(root);
}

TEST_CASE("ListAssets skips hidden names and puts directories first") {
	const auto root = makeTempDir();
	writeFile(root / "zeta.txt", "z");
	writeFile(root / ".hidden", "no");
	std::filesystem::create_directories(root / "alpha");
	writeFile(root / "alpha" / "in.txt", "i");

	FileSystem fs;
	fs.MountFolder("res", root);

	const std::vector<FileEntry> entries = editor::ListAssets(fs, "res://");
	REQUIRE(entries.size() == 2);
	CHECK(entries[0].name == "alpha");
	CHECK(entries[0].isDirectory);
	CHECK(entries[1].name == "zeta.txt");
	CHECK_FALSE(entries[1].isDirectory);

	std::filesystem::remove_all(root);
}

TEST_CASE("TryOpenAsset refuses non-scenes and missing scenes") {
	World world;
	ResourceRegistry resources;
	FileSystem fs;
	SceneManager scenes(world, resources, fs);

	CHECK_FALSE(editor::TryOpenAsset(scenes, "res://icon.png",
									 editor::AssetKind::Image));
}

TEST_CASE("TryOpenAsset opens tabs and activates only the focused scene") {
	const auto root = makeTempDir();
	FileSystem fs;
	fs.MountFolder("res", root);
	World world;
	ResourceRegistry resources;
	SceneManager scenes(world, resources, fs);

	Scene firstSeed = scenes.Create("First");
	REQUIRE(SaveSceneToFile(fs, "res://first.scene", firstSeed, resources) ==
			FsError::Ok);
	UnloadScene(firstSeed, resources);
	Scene secondSeed = scenes.Create("Second");
	REQUIRE(SaveSceneToFile(fs, "res://second.scene", secondSeed, resources) ==
			FsError::Ok);
	UnloadScene(secondSeed, resources);

	REQUIRE(editor::TryOpenAsset(scenes, "res://first.scene",
								 editor::AssetKind::Scene));
	Scene first = scenes.Current();
	REQUIRE(first.IsValid());
	CHECK(first.IsEnabled());

	REQUIRE(editor::TryOpenAsset(scenes, "res://second.scene",
								 editor::AssetKind::Scene));
	Scene second = scenes.Current();
	REQUIRE(second.IsValid());
	CHECK(second.SourcePath() == "res://second.scene");
	CHECK(second.IsEnabled());
	CHECK_FALSE(first.IsEnabled());
	CHECK(scenes.Loaded().size() == 2);

	REQUIRE(editor::TryOpenAsset(scenes, "res://first.scene",
								 editor::AssetKind::Scene));
	CHECK(scenes.Current().Root() == first.Root());
	CHECK(first.IsEnabled());
	CHECK_FALSE(second.IsEnabled());
	CHECK(scenes.Loaded().size() == 2);

	std::filesystem::remove_all(root);
}

TEST_CASE("Scene templates open as editable scenes with a 2D root") {
	const auto root = makeTempDir();
	FileSystem fs;
	fs.MountFolder("res", root);
	const auto created =
		editor::CreateAsset(fs, "res://", "Level #1", editor::AssetCreateKind::Scene);
	REQUIRE(created);
	CHECK(created.value == "res://Level #1.scene");
	World world;
	ResourceRegistry resources;
	SceneManager scenes(world, resources, fs);
	REQUIRE(editor::TryOpenAsset(scenes, created.value, editor::AssetKind::Scene));
	const Scene scene = scenes.EditorCurrent();
	REQUIRE(scene.IsValid());
	CHECK(scene.Name() == "Level #1");
	CHECK(scene.Root().Has<Transform2D>());
	CHECK(scene.Root().GetChildren().empty());
	std::filesystem::remove_all(root);
}

TEST_CASE("Script templates load and attach as Lua components") {
	const auto root = makeTempDir();
	FileSystem fs;
	fs.MountFolder("res", root);
	const auto created =
		editor::CreateAsset(fs, "res://", "2 Player", editor::AssetCreateKind::Script);
	REQUIRE(created);
	CHECK(created.value == "res://2 Player.lua");
	World world;
	ScriptServer scripts(world);
	REQUIRE(scripts.DoFile(fs, created.value));
	const auto types = scripts.ScriptComponentTypes(created.value);
	REQUIRE(types.size() == 1);
	CHECK(types.front().Name() == "Script_2_Player");
	Entity entity = world.CreateEntity();
	entity.SetName("Player");
	REQUIRE(scripts.AttachScript(entity, created.value));
	CHECK(entity.Has(types.front()));
	CHECK(scripts.CallComponentMethod(entity, types.front().Name(), "_start"));
	std::filesystem::remove_all(root);
}

TEST_CASE("Typed asset creation validates extensions and never replaces existing content") {
	const auto root = makeTempDir();
	FileSystem fs;
	fs.MountFolder("res", root);
	writeFile(root / "existing.scene", "keep this content");
	CHECK(editor::CreateAsset(fs, "res://", "existing", editor::AssetCreateKind::Scene).error ==
		  FsError::AlreadyExists);
	const auto original = fs.Load("res://existing.scene");
	REQUIRE(original);
	CHECK(std::string(reinterpret_cast<const char *>(original.value->Data()),
			  original.value->Size()) == "keep this content");
	for (const auto kind : {editor::AssetCreateKind::Scene, editor::AssetCreateKind::Script}) {
		CHECK(editor::CreateAsset(fs, "res://", "wrong.txt", kind).error == FsError::InvalidPath);
		CHECK(editor::CreateAsset(fs, "res://", "../escape", kind).error == FsError::InvalidPath);
		CHECK(editor::CreateAsset(fs, "res://", "", kind).error == FsError::InvalidPath);
	}
	const auto scene =
		editor::CreateAsset(fs, "res://", "Level.SCENE", editor::AssetCreateKind::Scene);
	REQUIRE(scene);
	CHECK(scene.value == "res://Level.SCENE");
	const auto script =
		editor::CreateAsset(fs, "res://", "Player.lua", editor::AssetCreateKind::Script);
	REQUIRE(script);
	CHECK(script.value == "res://Player.lua");
	CHECK_FALSE(std::filesystem::exists(root / "wrong.txt"));
	std::filesystem::remove_all(root);
}
