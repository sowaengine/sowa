#include "core/filesystem/file_system.hpp"
#include "ecs/components/transform2d.hpp"
#include "ecs/world.hpp"
#include "render/sprite_renderer2d.hpp"
#include "resource/image_texture.hpp"
#include "scene/packed_scene.hpp"
#include "scene/scene_serializer.hpp"
#include "scene/scene_snapshot.hpp"
#include "scene/tilemap_scenes.hpp"
#include <doctest/doctest.h>
#include <filesystem>
using namespace sowa;

namespace {
struct TileFixture {
	World world;
	FileSystem fs;
	ResourceRegistry resources;
	SpriteRenderer2D renderer;
	std::filesystem::path dir =
		std::filesystem::temp_directory_path() / "sowa_tilemap_prefab_tests";

	TileFixture() {
		std::filesystem::create_directories(dir);
		fs.MountFolder("tmp", dir);
		resources.SetContext({nullptr, &fs});
		resources.RegisterType<PackedScene>("PackedScene").Field("path", &PackedScene::path);
		renderer.Init(world, resources);
	}

	~TileFixture() {
		resources.Clear();
		std::filesystem::remove_all(dir);
	}

	ResourceID Prefab() {
		Scene prefab = CreateScene(world, "Coin");
		prefab.Root().Add<Transform2D>();
		prefab.Root().Get<Transform2D>()->position = {2, 3};
		prefab.CreateEntity("Child").Add<Transform2D>();
		REQUIRE(SaveSceneToFile(fs, "tmp://coin.scene", prefab, resources) == FsError::Ok);
		UnloadScene(prefab, resources);
		auto packed = resources.Create<PackedScene>();
		packed->path = "tmp://coin.scene";
		resources.Reload(packed->Id());
		return packed->Id();
	}
};
} // namespace

TEST_CASE("Tilemap prefabs spawn once, move with cells, survive duplication and erase cleanly") {
	TileFixture f;
	auto packed = f.Prefab();
	Scene scene = CreateScene(f.world, "Level");
	scene.Own(f.resources, packed);
	Entity entity = scene.CreateEntity("Map");
	entity.Add<Transform2D>();
	entity.Add<Tilemap>();
	const auto tile = entity.Get<Tilemap>()->AddScene(packed, "Coin");
	entity.Get<Tilemap>()->SetTile(0, 1, -1, tile);
	f.world.Progress(0);
	Entity coin = GetTileSceneEntity(entity, 0, 1, -1);
	REQUIRE(coin.IsAlive());
	CHECK(IsInstanceLocked(coin));
	CHECK(coin.Get<Transform2D>()->WorldPosition() == Vector2(26, -5));
	f.world.Progress(0);
	CHECK(GetTileSceneEntity(entity, 0, 1, -1) == coin);
	entity.Get<Tilemap>()->cellSize = {32, 32};
	f.world.Progress(0);
	CHECK(coin.Get<Transform2D>()->WorldPosition() == Vector2(50, -13));
	Entity duplicate = entity.Duplicate(scene.Root());
	f.world.Progress(0);
	Entity second = GetTileSceneEntity(duplicate, 0, 1, -1);
	REQUIRE(second.IsAlive());
	CHECK(second != coin);
	CHECK(duplicate.GetChildren().size() == 1);
	entity.Get<Tilemap>()->EraseTile(0, 1, -1);
	f.world.Progress(0);
	CHECK_FALSE(coin.IsAlive());
	CHECK(second.IsAlive());
	CHECK(entity.GetChildren().empty());
	duplicate.Remove<Tilemap>();
	f.world.Progress(0);
	CHECK_FALSE(second.IsAlive());
}

TEST_CASE("Tilemap consumed prefabs stay gone during play and return on snapshot restore") {
	TileFixture f;
	auto packed = f.Prefab();
	Scene scene = CreateScene(f.world, "Level");
	scene.Own(f.resources, packed);
	Entity entity = scene.CreateEntity("Map");
	entity.Add<Transform2D>();
	entity.Add<Tilemap>();
	auto tile = entity.Get<Tilemap>()->AddScene(packed);
	entity.Get<Tilemap>()->SetTile(0, 0, 0, tile);
	f.world.Progress(0);
	REQUIRE(TakeSnapshot(scene, f.resources));
	GetTileSceneEntity(entity, 0, 0, 0).Destroy();
	f.world.Progress(0);
	CHECK_FALSE(GetTileSceneEntity(entity, 0, 0, 0).IsAlive());
	REQUIRE(RestoreSnapshot(scene, f.resources));
	f.world.Progress(0);
	CHECK(GetTileSceneEntity(scene.Root().Find("Map"), 0, 0, 0).IsAlive());
}

TEST_CASE(
	"Tilemap prefab source replacement updates all instances and supports texture conversion") {
	TileFixture f;
	const auto first = f.Prefab();
	Scene replacement = CreateScene(f.world, "Enemy2");
	replacement.Root().Add<Transform2D>();
	REQUIRE(SaveSceneToFile(f.fs, "tmp://enemy2.scene", replacement, f.resources) == FsError::Ok);
	UnloadScene(replacement, f.resources);
	const auto second = f.resources.Create<PackedScene>();
	second->path = "tmp://enemy2.scene";
	f.resources.Reload(second->Id());
	Entity entity = f.world.CreateEntity();
	entity.Add<Tilemap>();
	entity.Add<Transform2D>();
	auto *map = entity.Get<Tilemap>();
	const auto tile = map->AddScene(first);
	map->SetTile(0, 0, 0, tile);
	map->SetTile(0, 1, 0, tile);
	f.world.Progress(0);
	const Entity old = GetTileSceneEntity(entity, 0, 0, 0);
	REQUIRE(old.IsAlive());
	REQUIRE(entity.Get<Tilemap>()->SetTileSource(tile, second->Id(), true));
	CHECK_FALSE(GetTileSceneEntity(entity, 0, 0, 0).IsAlive());
	f.world.Progress(0);
	CHECK_FALSE(old.IsAlive());
	CHECK(GetTileSceneEntity(entity, 0, 0, 0).GetName() == "Enemy2");
	CHECK(GetTileSceneEntity(entity, 0, 1, 0).GetName() == "Enemy2");
	REQUIRE(entity.Get<Tilemap>()->SetTileSource(tile, {123}));
	f.world.Progress(0);
	CHECK(entity.GetChildren().empty());
	CHECK(entity.Get<Tilemap>()->GetTile(0, 0, 0) == tile);
	REQUIRE(entity.Get<Tilemap>()->SetTileSource(tile, first, true));
	f.world.Progress(0);
	CHECK(GetTileSceneEntity(entity, 0, 0, 0).IsAlive());
}

TEST_CASE("Tilemap saves prefab palette and cells without generated children") {
	TileFixture f;
	auto packed = f.Prefab();
	Scene scene = CreateScene(f.world, "Level");
	scene.Own(f.resources, packed);
	Entity entity = scene.CreateEntity("Map");
	entity.Add<Transform2D>();
	entity.Add<Tilemap>();
	auto tile = entity.Get<Tilemap>()->AddScene(packed);
	entity.Get<Tilemap>()->SetTile(0, 0, 0, tile);
	f.world.Progress(0);
	Archive save = Archive::Save();
	REQUIRE(SaveScene(save, scene, f.resources));
	CHECK(save.ToString().find("Tile_0_0_0") == std::string::npos);
	Archive load = Archive::Load(save.ToString());
	Scene loaded = LoadScene(load, f.world, f.resources);
	CHECK_FALSE(load.HasErrors());
	f.world.Progress(0);
	Entity map = loaded.Root().Find("Map");
	CHECK(map.GetChildren().size() == 1);
	CHECK(GetTileSceneEntity(map, 0, 0, 0).IsAlive());
}

TEST_CASE("Tilemap prefab layers inherit visibility and scene disabling prevents new spawns") {
	TileFixture f;
	auto packed = f.Prefab();
	Scene scene = CreateScene(f.world, "Level");
	scene.Own(f.resources, packed);
	Entity entity = scene.CreateEntity("Map");
	entity.Add<Transform2D>();
	entity.Add<Tilemap>();
	auto tile = entity.Get<Tilemap>()->AddScene(packed);
	entity.Get<Tilemap>()->SetTile(0, 0, 0, tile);
	scene.SetEnabled(false);
	f.world.Progress(0);
	CHECK(entity.GetChildren().empty());
	scene.SetEnabled(true);
	f.world.Progress(0);
	Entity coin = GetTileSceneEntity(entity, 0, 0, 0);
	REQUIRE(coin.IsAlive());
	entity.Get<Tilemap>()->Layer(0)->visible = false;
	f.world.Progress(0);
	CHECK_FALSE(coin.IsVisibleInTree());
	entity.Get<Tilemap>()->Layer(0)->visible = true;
	f.world.Progress(0);
	CHECK(coin.IsVisibleInTree());
}

TEST_CASE("Tilemap recursive scene tiles stop at matching prefab ancestry") {
	TileFixture f;
	Scene prefab = CreateScene(f.world, "Recursive");
	prefab.Root().Add<Transform2D>();
	prefab.Root().Add<Tilemap>();
	const ResourceID id{99001};
	auto tile = prefab.Root().Get<Tilemap>()->AddScene(id);
	prefab.Root().Get<Tilemap>()->SetTile(0, 0, 0, tile);
	REQUIRE(SaveSceneToFile(f.fs, "tmp://recursive.scene", prefab, f.resources) == FsError::Ok);
	UnloadScene(prefab, f.resources);
	auto packed = f.resources.Create<PackedScene>(id);
	packed->path = "tmp://recursive.scene";
	f.resources.Reload(id);
	Entity instance = packed->Instantiate(f.world, f.resources);
	for (int i = 0; i < 4; ++i) {
		f.world.Progress(0);
	}
	CHECK(instance.GetChildren().empty());
}

TEST_CASE("Tilemap sample uses real sheet, individual image and nested prefab resources") {
	TileFixture f;
	f.fs.MountFolder("res", SOWA_EXAMPLE_DIR);
	f.resources.RegisterType<ImageTexture>("ImageTexture").Field("path", &ImageTexture::path);
	Scene scene = LoadSceneFromFile(f.fs, "res://scenes/tilemap.scene", f.world, f.resources);
	REQUIRE(scene.IsValid());
	Entity entity = scene.Root().Find("Map");
	REQUIRE(entity.Has<Tilemap>());
	CHECK(entity.Get<Tilemap>()->Tiles().size() == 4);
	CHECK(entity.Get<Tilemap>()->Layers().size() == 3);
	CHECK(entity.Get<Tilemap>()->Layer(0)->cells.size() == 180);
	const auto sheet = f.resources.Get<ImageTexture>({71001});
	REQUIRE(sheet);
	CHECK(sheet->Width() == 1988);
	CHECK(sheet->Height() == 1470);
	f.world.Progress(0);
	f.world.Progress(0);
	CHECK(GetTileSceneEntity(entity, 2, 5, 3).IsAlive());
	CHECK(GetTileSceneEntity(entity, 2, 5, 3).Find("Visual").Has<Tilemap>());
	CHECK(scene.ActiveCamera().IsAlive());
}
