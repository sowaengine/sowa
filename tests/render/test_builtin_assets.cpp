#include <doctest/doctest.h>

#include <filesystem>
#include <vector>

#include "core/filesystem/file_system.hpp"
#include "core/scope_exit.hpp"
#include "ecs/components/text2d.hpp"
#include "ecs/world.hpp"
#include "editor/project_manager.hpp"
#include "render/fullscreen_present.hpp"
#include "render/renderer2d.hpp"
#include "render/sprite_renderer2d.hpp"
#include "render/stub_graphics_device.hpp"
#include "resource/font.hpp"
#include "scene/scene_serializer.hpp"
#include "ui/components.hpp"
#include "ui/ui_system.hpp"

using namespace sowa;

namespace {

struct AssetDevice : StubGraphicsDevice {
	u32 buffers = 0;
	u32 pipelines = 0;
	u32 draws = 0;
	std::vector<u32> indices;

	BufferHandle CreateBuffer(BufferType type, BufferUsage, u32 size, const void *data) override {
		if (type == BufferType::Index && data != nullptr) {
			const auto *values = static_cast<const u32 *>(data);
			indices.assign(values, values + size / sizeof(u32));
		}
		return {++buffers, 1};
	}

	PipelineHandle CreatePipeline(const PipelineDesc &) override {
		return {++pipelines, 1};
	}

	void DrawIndexed(BufferHandle, BufferHandle, u32 count) override {
		draws += count;
	}
};

} // namespace

TEST_CASE("New projects render text and present without project shaders or fonts") {
	const auto root = std::filesystem::temp_directory_path() / "sowa_builtin_assets_test";
	std::filesystem::remove_all(root);
	const auto cleanup = MakeScopeExit([&] { std::filesystem::remove_all(root); });
	editor::ProjectManager manager(root / "projects.yaml");
	std::string error;
	REQUIRE(manager.Create(root / "game", "Empty Game", error));
	FileSystem fs;
	fs.MountFolder("res", root / "game");
	CHECK_FALSE(std::filesystem::exists(root / "game/shaders"));

	AssetDevice device;
	ResourceRegistry resources;
	resources.SetContext({&device, &fs, &resources});
	resources.RegisterType<Font>("Font")
		.Field("path", &Font::path)
		.Field("pixelSize", &Font::pixelSize);
	const auto font = resources.Create<Font>();
	REQUIRE(resources.Reload(font->Id()));
	REQUIRE(font->IsLoaded());
	REQUIRE(font->FindGlyph('A') != nullptr);
	CHECK(font->FindGlyph('A')->advance > 0.f);
	CHECK(font->FindGlyph('A')->size.x > 0.f);

	World world;
	Scene scene = LoadSceneFromFile(fs, "res://scenes/main.scene", world, resources);
	REQUIRE(scene.IsValid());
	Entity label = scene.CreateEntity("Label");
	label.Add(Transform2D{});
	Text2D text;
	text.text = "AB";
	label.Add(text);
	SpriteRenderer2D sprites;
	sprites.Init(world, resources);
	sprites.SetDefaultFont(font->Id());
	world.Progress(0.f);
	sprites.Collect();
	REQUIRE(sprites.DrawList().size() == 2);
	CHECK(sprites.DrawList()[0].texture == font->Atlas());
	CHECK(sprites.DrawList()[0].localRect.w > 0.f);

	Renderer2D renderer;
	REQUIRE(renderer.Init(device, fs));
	EntityPickMap picks;
	sprites.Draw(renderer, picks);
	CHECK(device.draws == 12);
	FullscreenPresent present;
	REQUIRE(present.Init(device, fs));
	CHECK(device.indices == std::vector<u32>{0, 1, 2, 0, 2, 3});
	present.Draw({42, 1});
	CHECK(device.draws == 18);
	Entity canvas = CreateUI(scene.Root(), "UICanvas");
	canvas.Get<UICanvas>()->space = UISpace::Screen;
	Entity control = CreateUI(canvas, "UILabel");
	control.Get<UILabel>()->text = "AB";
	UISystem ui;
	ui.Init(world, resources);
	ui.SetDefaultFont(font->Id());
	ui.Layout({800, 600});
	ui.Draw(renderer, picks);
	CHECK(device.draws == 30);

	label.Get<Text2D>()->font = ResourceID{123456789};
	sprites.Collect();
	CHECK(sprites.DrawList().empty());
}

TEST_CASE("Embedded fonts reload without filesystem access and release their atlas") {
	AssetDevice device;
	ResourceRegistry resources;
	resources.SetContext({&device, nullptr, &resources});
	resources.RegisterType<Font>("Font")
		.Field("path", &Font::path)
		.Field("pixelSize", &Font::pixelSize);
	const auto font = resources.Create<Font>();
	REQUIRE(resources.Reload(font->Id()));
	REQUIRE(font->IsLoaded());
	const float advance = font->FindGlyph('A')->advance;
	font->pixelSize = 64;
	REQUIRE(resources.Reload(font->Id()));
	REQUIRE(font->IsLoaded());
	CHECK(device.textureDestroyCount == 1);
	CHECK(font->FindGlyph('A')->advance == doctest::Approx(advance * 2));
	font->path = "res://missing.ttf";
	REQUIRE(resources.Reload(font->Id()));
	CHECK_FALSE(font->HasGlyphs());
	CHECK_FALSE(font->IsLoaded());
	CHECK(device.textureDestroyCount == 2);
}

TEST_CASE("Explicit missing shader overrides fail instead of silently using defaults") {
	AssetDevice device;
	FileSystem fs;
	Renderer2D renderer;
	FullscreenPresent present;
	CHECK_FALSE(renderer.Init(device, fs, "res://missing.glsl"));
	CHECK_FALSE(present.Init(device, fs, "res://missing.glsl"));
	CHECK_FALSE(device.createCalled);
	CHECK(device.buffers == 0);
}

TEST_CASE("Example text scene renders with the embedded default font") {
	AssetDevice device;
	FileSystem fs;
	fs.MountFolder("res", SOWA_EXAMPLE_DIR);
	ResourceRegistry resources;
	resources.SetContext({&device, &fs, &resources});
	resources.RegisterType<Font>("Font")
		.Field("path", &Font::path)
		.Field("pixelSize", &Font::pixelSize);
	const auto font = resources.Create<Font>();
	REQUIRE(resources.Reload(font->Id()));
	REQUIRE(font->IsLoaded());

	World world;
	SpriteRenderer2D sprites;
	sprites.Init(world, resources);
	sprites.SetDefaultFont(font->Id());
	Scene scene = LoadSceneFromFile(fs, "res://scenes/text.scene", world, resources);
	REQUIRE(scene.IsValid());
	const auto *title = scene.Root().Find("Title").Get<Text2D>();
	REQUIRE(title != nullptr);
	CHECK_FALSE(title->font);

	world.Progress(0.f);
	sprites.Collect();
	REQUIRE_FALSE(sprites.DrawList().empty());
	for (const auto &draw : sprites.DrawList()) {
		CHECK(draw.texture == font->Atlas());
	}
}
