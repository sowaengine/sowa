#include "core/filesystem/file_system.hpp"
#include "ecs/component_serializer.hpp"
#include "ecs/components/tilemap.hpp"
#include "ecs/components/transform2d.hpp"
#include "ecs/world.hpp"
#include "editor/scene_commands.hpp"
#include "editor/tilemap_tools.hpp"
#include "render/renderer2d.hpp"
#include "render/sprite_renderer2d.hpp"
#include "render/stub_graphics_device.hpp"
#include "resource/image_texture.hpp"
#include "scene/packed_scene.hpp"
#include "scene/scene_serializer.hpp"
#include "scene/scene_snapshot.hpp"
#include "script/script_server.hpp"
#include <cstring>
#include <doctest/doctest.h>
#include <glm/gtc/matrix_transform.hpp>
#include <limits>
using namespace sowa;

TEST_CASE("Tilemap sparse cells, stable IDs and layer edits") {
	Tilemap map;
	const auto grass = map.AddTexture({1}, "grass"), flower = map.AddTexture({2}, "flower");
	REQUIRE(map.SetTile(0, -100, 7, grass));
	CHECK(map.GetTile(0, -100, 7) == grass);
	CHECK_FALSE(map.SetTile(0, 0, 0, 999));
	CHECK_FALSE(map.SetTile(-1, 0, 0, grass));
	CHECK(map.GetTile(4, 0, 0) == 0);
	const auto overlay = map.AddLayer("Flowers");
	map.SetTile(overlay, -100, 7, flower);
	map.MoveLayer(1, 0);
	CHECK(map.GetTile(0, -100, 7) == flower);
	CHECK(map.GetTile(1, -100, 7) == grass);
	map.RemoveTile(grass);
	CHECK(map.GetTile(1, -100, 7) == 0);
	CHECK(map.AddScene({3}, "coin") > flower);
	map.RemoveLayer(0);
	CHECK(map.Layers().size() == 1);
	map.Clear();
	CHECK(map.Layers()[0].cells.empty());
}

TEST_CASE("Tilemap atlas slicing uses top-left regions and handles margins and flip") {
	Tilemap map;
	const auto ids = map.AddSheet({1}, {37, 37}, {16, 16}, 2, 1);
	REQUIRE(ids.size() == 4);
	const auto *tile = map.FindTile(ids[3]);
	REQUIRE(tile);
	CHECK(tile->region.x == 19);
	CHECK(tile->region.y == 19);
	const auto uv = TileUV(*tile, {37, 37});
	CHECK(uv.y == doctest::Approx(2.f / 37.f));
	CHECK(uv.w == doctest::Approx(16.f / 37.f));
	const auto flipped = TileUV(*tile, {37, 37}, false);
	CHECK(flipped.y == doctest::Approx(35.f / 37.f));
	CHECK(flipped.h == doctest::Approx(-16.f / 37.f));
	CHECK(map.AddSheet({1}, {16, 16}, {0, 16}).empty());
	CHECK(map.AddSheet({1}, {16, 16}, {1e-30f, 1e-30f}).empty());
	CHECK(map.AddSheet({1}, {16, 16}, {16, 16}, -1).empty());
	CHECK(TileUV(*tile, {8, 8}).w == 0);
}

TEST_CASE(
	"Tilemap coordinate conversion floors negative positions and rejects invalid transforms") {
	Tilemap map;
	REQUIRE(map.LocalToCell({-.1f, -16.1f}));
	CHECK(map.LocalToCell({-.1f, -16.1f})->x == -1);
	CHECK(map.LocalToCell({-.1f, -16.1f})->y == -2);
	CHECK(map.CellToLocal(-1, 2) == Vector2(-8, 40));
	const auto transform = glm::translate(glm::mat4(1), glm::vec3(30, 20, 0)) *
						   glm::rotate(glm::mat4(1), glm::radians(90.f), glm::vec3(0, 0, 1));
	const auto vp = glm::ortho(0.f, 100.f, 0.f, 100.f, -1.f, 1.f);
	// local cell (-1,0) center (-8,8) => world (22,12) => image (44,176).
	const auto cell =
		editor::TilemapViewportCell(map, transform, vp, Rect(0, 0, 200, 200), {44, 176});
	REQUIRE(cell);
	CHECK(cell->x == -1);
	CHECK(cell->y == 0);
	CHECK_FALSE(
		editor::TilemapViewportCell(map, glm::mat4(0), vp, Rect(0, 0, 200, 200), {44, 176}));
	map.cellSize.x = 0;
	CHECK_FALSE(map.LocalToCell({0, 0}));
	map.cellSize.x = 16;
	CHECK_FALSE(map.LocalToCell({std::numeric_limits<float>::infinity(), 0}));
}

TEST_CASE("Tilemap paint interpolates and erases only the active layer") {
	Tilemap map;
	auto tile = map.AddTexture({1});
	map.AddLayer("Overlay");
	REQUIRE(editor::PaintTileLine(map, 1, {-2, -2}, {3, 3}, tile));
	CHECK(map.Layers()[1].cells.size() == 6);
	CHECK(map.Layers()[0].cells.empty());
	for (int i = -2; i <= 3; ++i) {
		CHECK(map.GetTile(1, i, i) == tile);
	}
	CHECK(editor::PaintTileLine(map, 1, {-2, -2}, {3, 3}, 0));
	CHECK(map.Layers()[1].cells.empty());
}

TEST_CASE("Tilemap sheets preserve coordinates and painted IDs across source changes") {
	Tilemap map;
	const auto tiles = map.AddSheet({1}, {32, 32}, {16, 16});
	REQUIRE(tiles.size() == 4);
	const i32 sheet = map.SheetForTile(tiles[0])->id;
	map.SetTile(0, 3, -2, tiles[2]);
	REQUIRE(map.RefreshSheet(sheet, {48, 32}));
	CHECK(map.Tiles().size() == 6);
	CHECK(map.FindSheet(sheet)->tiles.at({0, 1}) == tiles[2]);
	CHECK(map.FindTile(tiles[2])->region.x == 0);
	CHECK(map.FindTile(tiles[2])->region.y == 16);
	CHECK(map.GetTile(0, 3, -2) == tiles[2]);
	CHECK_FALSE(map.RefreshSheet(sheet, {48, 32}));
	REQUIRE(map.RefreshSheet(sheet, {16, 16}));
	CHECK(TileUV(*map.FindTile(tiles[2]), {16, 16}).w == 0);
	CHECK(map.GetTile(0, 3, -2) == tiles[2]);
	REQUIRE(map.RefreshSheet(sheet, {32, 32}));
	CHECK(map.FindSheet(sheet)->tiles.at({0, 1}) == tiles[2]);
	CHECK(TileUV(*map.FindTile(tiles[2]), {32, 32}).w == .5f);
	REQUIRE(map.UpdateSheet(sheet, {2}, {19, 19}, {8, 8}, 1, 1));
	CHECK(map.FindTile(tiles[2])->resource == ResourceID{2});
	CHECK(map.FindTile(tiles[2])->region.y == 10);
	CHECK(map.FindTile(tiles[2])->region.h == 8);
	REQUIRE(map.RefreshSheet(sheet, {18, 18}));
	CHECK(TileUV(*map.FindTile(tiles[2]), {18, 18}).w == 0); // respect the outer margin
	CHECK_FALSE(map.UpdateSheet(sheet, {3}, {32, 32}, {0, 0}));
	CHECK(map.FindTile(tiles[2])->resource == ResourceID{2});
}

TEST_CASE("Tilemap individual source overrides and deletions survive sheet refresh") {
	Tilemap map;
	const auto tiles = map.AddSheet({1}, {32, 16}, {16, 16});
	const i32 sheet = map.SheetForTile(tiles[0])->id;
	map.SetTile(0, 1, 2, tiles[0]);
	map.SetTile(map.AddLayer("Objects"), 8, 9, tiles[0]);
	REQUIRE(map.SetTileSource(tiles[0], {9}, true));
	CHECK(map.FindTile(tiles[0])->scene);
	CHECK(map.SheetForTile(tiles[0]) == nullptr);
	CHECK(map.GetTile(0, 1, 2) == tiles[0]);
	CHECK(map.GetTile(1, 8, 9) == tiles[0]);
	REQUIRE(map.RemoveTile(tiles[1]));
	REQUIRE(map.RefreshSheet(sheet, {48, 16}));
	CHECK(map.Tiles().size() == 2);
	CHECK(map.FindTile(tiles[0])->resource == ResourceID{9});
	CHECK(map.FindTile(tiles[1]) == nullptr);
	REQUIRE(map.SetTileSource(tiles[0], {10}));
	CHECK_FALSE(map.FindTile(tiles[0])->scene);
	CHECK(map.FindTile(tiles[0])->region.w == 0);
	CHECK_FALSE(map.SetTileSource(tiles[0], {}));
	CHECK(map.FindTile(tiles[0])->resource == ResourceID{10});
}

TEST_CASE("Tilemap sheets and removed slots survive serialization and undo") {
	World world;
	Scene scene = CreateScene(world, "Sheet");
	Entity entity = scene.CreateEntity("Map");
	entity.Add<Tilemap>();
	auto *map = entity.Get<Tilemap>();
	const auto tiles = map->AddSheet({1}, {32, 32}, {16, 16});
	const auto sheet = map->SheetForTile(tiles[0])->id;
	map->SetTile(0, 2, 3, tiles[0]);
	map->RemoveTile(tiles[1]);
	Archive save = Archive::Save();
	map->Serialize(save);
	Archive load = Archive::Load(save.ToString());
	Tilemap restored;
	restored.Serialize(load);
	REQUIRE_FALSE(load.HasErrors());
	REQUIRE(restored.Sheets().size() == 1);
	REQUIRE(restored.RefreshSheet(sheet, {48, 32}));
	CHECK(restored.Tiles().size() == 5);
	CHECK(restored.FindTile(tiles[1]) == nullptr);
	CHECK(restored.GetTile(0, 2, 3) == tiles[0]);
	std::vector<editor::SceneDocumentSaveState> states;
	editor::UpdateSceneDocumentStates(states, {scene});
	map = entity.Get<Tilemap>();
	Tilemap before = *map;
	REQUIRE(map->UpdateSheet(sheet, {2}, {32, 32}, {8, 8}));
	REQUIRE(editor::RecordTilemapEdit(states, scene, entity, before, *map));
	REQUIRE(editor::UndoSceneDocument(states, scene));
	CHECK(entity.Get<Tilemap>()->FindTile(tiles[0])->resource == ResourceID{1});
	CHECK(entity.Get<Tilemap>()->FindSheet(sheet)->tileSize == Vector2(16, 16));
	REQUIRE(editor::RedoSceneDocument(states, scene));
	CHECK(entity.Get<Tilemap>()->FindTile(tiles[0])->resource == ResourceID{2});
	CHECK(entity.Get<Tilemap>()->GetTile(0, 2, 3) == tiles[0]);
}

TEST_CASE("Tilemap renderer reslices sheets after texture reload") {
	World world;
	ResourceRegistry resources;
	resources.RegisterType<ImageTexture>("ImageTexture");
	auto texture = resources.Create<ImageTexture>();
	texture->Adopt(resources.Context(), {4, 1}, 32, 32);
	SpriteRenderer2D renderer;
	renderer.Init(world, resources);
	Entity entity = world.CreateEntity();
	entity.Add<Transform2D>();
	entity.Add<Tilemap>();
	auto *map = entity.Get<Tilemap>();
	const auto tiles = map->AddSheet(texture->Id(), texture->Size(), {16, 16});
	map->SetTile(0, 0, 0, tiles[3]);
	texture->Adopt(resources.Context(), {5, 1}, 48, 32);
	renderer.Collect();
	CHECK(map->Tiles().size() == 6);
	REQUIRE(renderer.DrawList().size() == 1);
	CHECK(renderer.DrawList()[0].uvRect.x == doctest::Approx(1.f / 3.f));
	texture->Adopt(resources.Context(), {6, 1}, 16, 16);
	renderer.Collect();
	CHECK(renderer.DrawList().empty());
	CHECK(map->GetTile(0, 0, 0) == tiles[3]);
}

TEST_CASE("Tilemap source picker loads and reuses project assets and rejects invalid files") {
	FileSystem fs;
	fs.MountFolder("res", SOWA_EXAMPLE_DIR);
	ResourceRegistry resources;
	resources.SetContext({nullptr, &fs});
	resources.RegisterType<ImageTexture>("ImageTexture");
	resources.RegisterType<PackedScene>("PackedScene");
	const auto image =
		editor::LoadTileAsset(resources, fs, "res://textures/tile_334.png", false, "");
	REQUIRE(image.id.IsValid());
	CHECK(image.created);
	const auto same =
		editor::LoadTileAsset(resources, fs, "res://textures/./tile_334.png", false, "");
	CHECK(same.id == image.id);
	CHECK_FALSE(same.created);
	const auto prefab = editor::LoadTileAsset(resources, fs, "res://scenes/tilemap_coin.scene",
											  true, "res://scenes/tilemap.scene");
	CHECK(prefab.id.IsValid());
	CHECK_FALSE(
		editor::LoadTileAsset(resources, fs, "res://textures/missing.png", false, "").id.IsValid());
	CHECK_FALSE(
		editor::LoadTileAsset(resources, fs, "res://textures/tile_334.png", true, "").id.IsValid());
	CHECK_FALSE(editor::LoadTileAsset(resources, fs, "res://scenes/tilemap.scene", true,
									  "res://scenes/./tilemap.scene")
					.id.IsValid());
	CHECK(resources.Get<ImageTexture>(image.id)->Width() > 0);
}

TEST_CASE("Tilemap serialization, duplication and snapshot preserve independent data") {
	World world;
	ResourceRegistry resources;
	Scene scene = CreateScene(world, "MapTest");
	Entity entity = scene.CreateEntity("Map");
	entity.Add<Tilemap>();
	auto *map = entity.Get<Tilemap>();
	const auto id = map->AddTexture({33}, "grass");
	map->AddLayer("Flowers");
	map->Layer(1)->visible = false;
	map->SetTile(1, -7, 12, id);
	Archive archive = Archive::Save();
	REQUIRE(SaveScene(archive, scene, resources));
	Archive load = Archive::Load(archive.ToString());
	Scene copy = LoadScene(load, world, resources);
	CHECK_FALSE(load.HasErrors());
	REQUIRE(copy.IsValid());
	auto *loaded = copy.Root().Find("Map").Get<Tilemap>();
	REQUIRE(loaded);
	CHECK(loaded->GetTile(1, -7, 12) == id);
	CHECK_FALSE(loaded->Layer(1)->visible);
	Entity duplicate = entity.Duplicate(scene.Root());
	duplicate.Get<Tilemap>()->Clear();
	CHECK(entity.Get<Tilemap>()->GetTile(1, -7, 12) == id);
	REQUIRE(TakeSnapshot(scene, resources));
	entity.Get<Tilemap>()->RemoveTile(id);
	REQUIRE(RestoreSnapshot(scene, resources));
	CHECK(scene.Root().Find("Map").Get<Tilemap>()->GetTile(1, -7, 12) == id);
}

TEST_CASE("Tilemap malformed data reports errors and discards dangling tile IDs") {
	Tilemap map;
	Archive archive = Archive::Load(R"(
cellSize: [0, -2]
tiles:
 - {id: 1, resource: 7}
 - {id: 1, resource: 8}
layers:
 - id: 0
   cells: [{x: 3, y: 4, tile: 999}]
)");
	map.Serialize(archive);
	CHECK(archive.HasErrors());
	CHECK(map.ValidCellSize());
	CHECK(map.Tiles().size() == 1);
	CHECK(map.GetTile(0, 3, 4) == 0);
	CHECK(map.AddTexture({9}) == 2);
}

TEST_CASE("Tilemap Lua methods survive storage migration and removed components") {
	World world;
	ScriptServer scripts(world);
	REQUIRE(scripts.DoString(R"(
		e=world:create_entity('Map')
		m=e:add('Tilemap')
		assert(type(m.get_used_cells)=='function')
		local grass=m:add_texture(1,'Grass')
		local ids=m:add_sheet(2,Vector2(32,32),Vector2(16,16))
		assert(#ids==4)
		assert(m:update_sheet(m:get_tile_definition(ids[1]).sheet,3,Vector2(48,32),Vector2(16,16)))
		assert(m:get_tile_definition(ids[1]).resource==3)
		assert(m:set_tile_source(ids[1],4,true))
		assert(m:get_tile_definition(ids[1]).scene)
		assert(m:get_tile_definition(ids[1]).sheet==0)
		assert(m:set_tile_name(ids[1],'Enemy'))
		assert(m:get_tile_definition(ids[1]).name=='Enemy')
		assert(m:layer_count()==1)
		local layer=m:add_layer('Flowers')
		assert(layer==1)
		assert(m:set_tile(layer,-2,3,grass))
		assert(m:get_tile(layer,-2,3)==grass)
		assert(m:get_used_cells(layer)[1].tile==grass)
		assert(m:local_to_cell(Vector2(-1,0)).x==-1)
		assert(m:cell_to_local(-1,0).x==-8)
		m:set_layer_visible(layer,false)
		assert(not m:get_layer(layer).visible)
		e:add('Transform2D')
		assert(m:get_tile(layer,-2,3)==grass)
		assert(m:erase_tile(layer,-2,3))
		local get=m.get_tile
		e:remove('Tilemap')
		assert(get(m,0,0,0)==0)
	)"));
}

TEST_CASE("Tilemap undo redo survives play snapshot restoration") {
	World world;
	ResourceRegistry resources;
	Scene scene = CreateScene(world, "UndoMap");
	Entity entity = scene.CreateEntity("Map");
	entity.Add<Tilemap>();
	auto id = entity.Get<Tilemap>()->AddTexture({1});
	std::vector<editor::SceneDocumentSaveState> states;
	editor::UpdateSceneDocumentStates(states, {scene});
	Tilemap before = *entity.Get<Tilemap>();
	entity.Get<Tilemap>()->SetTile(0, 4, -2, id);
	REQUIRE(editor::RecordTilemapEdit(states, scene, entity, before, *entity.Get<Tilemap>()));
	REQUIRE(editor::UndoSceneDocument(states, scene));
	CHECK(entity.Get<Tilemap>()->GetTile(0, 4, -2) == 0);
	REQUIRE(editor::RedoSceneDocument(states, scene));
	CHECK(entity.Get<Tilemap>()->GetTile(0, 4, -2) == id);
	REQUIRE(TakeSnapshot(scene, resources));
	REQUIRE(RestoreSnapshot(scene, resources));
	editor::UpdateSceneDocumentStates(states, {scene});
	REQUIRE(editor::UndoSceneDocument(states, scene));
	CHECK(scene.Root().Find("Map").Get<Tilemap>()->GetTile(0, 4, -2) == 0);
}

TEST_CASE("Tilemap renderer batches regions with layer ordering and entity picking identity") {
	World world;
	ResourceRegistry resources;
	resources.RegisterType<ImageTexture>("ImageTexture");
	const auto texture = resources.Create<ImageTexture>();
	texture->Adopt(resources.Context(), {4, 1}, 32, 32);
	SpriteRenderer2D renderer;
	renderer.Init(world, resources);
	Entity entity = world.CreateEntity();
	entity.Add<Transform2D>();
	entity.Add<Tilemap>();
	auto *map = entity.Get<Tilemap>();
	auto ids = map->AddSheet(texture->Id(), {32, 32}, {16, 16});
	map->SetTile(0, -1, 0, ids[0]);
	map->AddLayer("Flowers");
	map->SetTile(1, -1, 0, ids[3]);
	world.Progress(0);
	renderer.Collect();
	REQUIRE(renderer.DrawList().size() == 2);
	CHECK(renderer.DrawList()[0].entity == entity);
	CHECK(renderer.DrawList()[0].localRect.x == -16);
	CHECK(renderer.DrawList()[0].uvRect.y == .5f);
	CHECK(renderer.DrawList()[1].uvRect.y == 0);
	CHECK(renderer.DrawList()[0].key < renderer.DrawList()[1].key);
	entity.Get<Tilemap>()->Layer(1)->visible = false;
	renderer.Collect();
	CHECK(renderer.DrawList().size() == 1);
	entity.SetVisible(false);
	renderer.Collect();
	CHECK(renderer.DrawList().empty());
}

TEST_CASE("Tilemap atlas quads submit transformed vertices UVs and picking tokens") {
	struct Vertex {
		float x, y, z, u, v, r, g, b, a, texture;
		u32 token;
		float clipLeft, clipBottom, clipRight, clipTop;
	};

	struct Device : StubGraphicsDevice {
		std::vector<Vertex> vertices;

		PipelineHandle CreatePipeline(const PipelineDesc &) override { return {1, 1}; }

		BufferHandle CreateBuffer(BufferType type, BufferUsage, u32, const void *) override {
			return {type == BufferType::Vertex ? 1u : 2u, 1};
		}

		void UpdateBuffer(BufferHandle, const void *data, u32 size, u32) override {
			vertices.resize(size / sizeof(Vertex));
			std::memcpy(vertices.data(), data, size);
		}
	} device;

	FileSystem fs;
	fs.MountFolder("res", SOWA_EXAMPLE_DIR);
	Renderer2D renderer;
	REQUIRE(renderer.Init(device, fs));
	const auto transform = glm::translate(glm::mat4(1), glm::vec3(10, 20, 0)) *
						   glm::rotate(glm::mat4(1), glm::radians(90.f), glm::vec3(0, 0, 1));
	renderer.Begin();
	renderer.DrawQuad(transform, Rect(0, 0, 16, 16), Rect(.25f, .5f, .25f, .25f),
					  TextureHandle{2, 1}, Color{}, 0, 123);
	renderer.End();
	REQUIRE(device.vertices.size() == 4);
	CHECK(device.vertices[0].x == doctest::Approx(-6));
	CHECK(device.vertices[0].y == doctest::Approx(20));
	CHECK(device.vertices[0].u == .25f);
	CHECK(device.vertices[0].v == .75f);
	CHECK(device.vertices[2].u == .5f);
	CHECK(device.vertices[2].v == .5f);
	for (const auto &vertex : device.vertices) {
		CHECK(vertex.token == 123);
	}
}
