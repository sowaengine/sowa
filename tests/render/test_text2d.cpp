#include <limits>

#include <doctest/doctest.h>

#include "core/filesystem/file_system.hpp"
#include "ecs/components/text2d.hpp"
#include "ecs/visibility.hpp"
#include "ecs/world.hpp"
#include "editor/hierarchy.hpp"
#include "render/sprite_renderer2d.hpp"
#include "render/stub_graphics_device.hpp"
#include "resource/font.hpp"
#include "resource/image_texture.hpp"
#include "scene/scene_serializer.hpp"
#include "script/script_server.hpp"

using namespace sowa;

namespace {
struct Fixture {
	StubGraphicsDevice device;
	FileSystem fs;
	ResourceRegistry resources;
	Ref<Font> font;

	Fixture() {
		fs.MountFolder("res", SOWA_EXAMPLE_DIR);
		resources.SetContext({&device, &fs, &resources});
		resources.RegisterType<Font>("Font")
			.Field("path", &Font::path)
			.Field("pixelSize", &Font::pixelSize);
		resources.RegisterType<ImageTexture>("ImageTexture");
		font = resources.Create<Font>();
		resources.Reload(font->Id());
		REQUIRE(font->IsLoaded());
	}
};
} // namespace

TEST_CASE("Text2D: layout anchors multiline blocks using font metrics") {
	Fixture f;
	TextLayoutOptions options;
	options.offset = {15.f, 50.f};
	options.lineSpacing = 1.5f;
	const auto baseline = LayoutTextBlock(*f.font, "WW\ni", options);
	REQUIRE(baseline.quads.size() == 3);
	CHECK(baseline.lineCount == 2);
	const auto *w = f.font->FindGlyph('W');
	const auto *i = f.font->FindGlyph('i');
	CHECK(baseline.bounds.w == doctest::Approx(w->advance * 2));
	CHECK(baseline.quads[2].rect.y ==
		  doctest::Approx(50.f - f.font->LineHeight() * 1.5f + i->bearing.y - i->size.y));

	options.horizontalAlignment = TextHorizontalAlignment::Center;
	options.verticalAlignment = TextVerticalAlignment::Center;
	const auto centered = LayoutTextBlock(*f.font, "WW\ni", options);
	CHECK(centered.bounds.x + centered.bounds.w / 2 == doctest::Approx(15.f));
	CHECK(centered.bounds.y + centered.bounds.h / 2 == doctest::Approx(50.f));
	CHECK(centered.quads[0].rect.x == doctest::Approx(15.f - w->advance + w->bearing.x));
	CHECK(centered.quads[2].rect.x == doctest::Approx(15.f - i->advance / 2 + i->bearing.x));

	options.horizontalAlignment = TextHorizontalAlignment::Right;
	options.verticalAlignment = TextVerticalAlignment::Top;
	const auto top = LayoutTextBlock(*f.font, "WW\ni", options);
	CHECK(top.bounds.x + top.bounds.w == doctest::Approx(15.f));
	CHECK(top.bounds.y + top.bounds.h == doctest::Approx(50.f));
	options.verticalAlignment = TextVerticalAlignment::Bottom;
	const auto bottom = LayoutTextBlock(*f.font, "WW\ni", options);
	CHECK(bottom.bounds.y == doctest::Approx(50.f));
}

TEST_CASE("Text2D: sizing scales positions and bounds while preserving atlas UVs") {
	Fixture f;
	const auto native = LayoutTextBlock(*f.font, "Ay\nB");
	TextLayoutOptions options;
	options.fontSize = 64.f;
	const auto large = LayoutTextBlock(*f.font, "Ay\nB", options);
	REQUIRE(large.quads.size() == native.quads.size());
	CHECK(large.bounds.w == doctest::Approx(native.bounds.w * 2));
	CHECK(large.bounds.h == doctest::Approx(native.bounds.h * 2));
	for (size_t i = 0; i < native.quads.size(); ++i) {
		CHECK(large.quads[i].rect.w == doctest::Approx(native.quads[i].rect.w * 2));
		CHECK(large.quads[i].rect.y == doctest::Approx(native.quads[i].rect.y * 2));
		CHECK(large.quads[i].uvRect.x == native.quads[i].uvRect.x);
		CHECK(large.quads[i].uvRect.h == native.quads[i].uvRect.h);
	}
}

TEST_CASE("Text2D: wrapping respects words and splits oversized words without losing glyphs") {
	Fixture f;
	const float word = f.font->FindGlyph('A')->advance * 2;
	TextLayoutOptions options;
	options.wrapWidth = word + 0.1f;
	const auto words = LayoutTextBlock(*f.font, "AA AA", options);
	CHECK(words.lineCount == 2);
	REQUIRE(words.quads.size() == 4);
	CHECK(words.quads[2].rect.x == doctest::Approx(words.quads[0].rect.x));
	CHECK(words.bounds.w <= options.wrapWidth);
	CHECK(LayoutTextBlock(*f.font, "AA   \nAA", options).lineCount == 2);
	CHECK(LayoutTextBlock(*f.font, "AA   \n", options).lineCount == 2);
	const auto longWord = LayoutTextBlock(*f.font, "AAAAA", options);
	CHECK(longWord.lineCount == 3);
	CHECK(longWord.quads.size() == 5);
	CHECK(longWord.bounds.w <= options.wrapWidth);
	options.wrapWidth = 0.01f;
	const auto narrow = LayoutTextBlock(*f.font, "AAA", options);
	CHECK(narrow.lineCount == 3);
	CHECK(narrow.quads.size() == 3);
	options.fontSize = 64.f;
	options.wrapWidth = word + 0.1f;
	CHECK(LayoutTextBlock(*f.font, "AA", options).lineCount == 2);
}

TEST_CASE("Text2D: whitespace, missing glyphs and invalid inputs are safe") {
	Fixture f;
	const auto crlf = LayoutTextBlock(*f.font, "A\r\n\r\nA\n");
	CHECK(crlf.lineCount == 4);
	CHECK(crlf.quads.size() == 2);
	const auto tab = LayoutTextBlock(*f.font, "A\tB");
	const auto spaces = LayoutTextBlock(*f.font, "A    B");
	CHECK(tab.bounds.w == doctest::Approx(spaces.bounds.w));
	CHECK(LayoutTextBlock(*f.font, "A\xc3\xa9").quads.size() == 1);
	CHECK(LayoutTextBlock(*f.font, "").lineCount == 0);
	CHECK(LayoutTextBlock(*f.font, "   ").quads.empty());
	TextLayoutOptions options;
	options.lineSpacing = -1.f;
	CHECK(LayoutTextBlock(*f.font, "A", options).quads.empty());
	options.lineSpacing = 1.f;
	options.fontSize = std::numeric_limits<float>::quiet_NaN();
	CHECK(LayoutTextBlock(*f.font, "A", options).quads.empty());
	f.font->Unload(f.resources.Context());
	CHECK(LayoutTextBlock(*f.font, "A").quads.empty());
}

TEST_CASE("Text2D: collection shares sprite ordering, transforms, tint and entity picking") {
	Fixture f;
	World world;
	SpriteRenderer2D renderer;
	renderer.Init(world, f.resources);
	Entity parent = world.CreateEntity();
	parent.Add(Transform2D{{10, 20}, 30.f, {2, 3}});
	parent.Add(RenderOrder{7, ZIndexMode::Relative, 2, RenderLayerMode::Explicit});
	Entity label = editor::SpawnChild(parent, editor::EntityArchetype::Text2D);
	REQUIRE(label.Has<Transform2D>());
	auto *text = label.Get<Text2D>();
	REQUIRE(text != nullptr);
	text->font = f.font->Id();
	text->text = "A B";
	text->modulate = Color(0.2f, 0.4f, 0.6f, 0.8f);
	label.Get<Transform2D>()->position = {4.f, 5.f};
	Entity sprite = world.CreateEntity();
	sprite.Add(Transform2D{});
	auto texture = f.resources.Create<ImageTexture>();
	texture->Adopt(f.resources.Context(), {90, 1}, 16, 16);
	Sprite visual;
	visual.texture = texture->Id();
	sprite.Add(visual);
	sprite.Add(RenderOrder{100}); // layer 0 must still be behind layer 2 text
	world.Progress(0.f);
	renderer.Collect();
	REQUIRE(renderer.DrawList().size() == 3);
	CHECK(renderer.DrawList()[0].entity == sprite);
	for (size_t i = 1; i < renderer.DrawList().size(); ++i) {
		const auto &item = renderer.DrawList()[i];
		CHECK(item.entity == label);
		CHECK(item.texture == f.font->Atlas());
		CHECK(item.transform == label.Get<Transform2D>()->worldMatrix);
		CHECK(item.key.layer == 2);
		CHECK(item.key.zIndex == 7);
		CHECK(item.modulate.a == doctest::Approx(0.8f));
		CHECK(item.uvRect.w < 1.f);
		CHECK(item.alphaCutoff < 0.01f);
	}
	// Live edits and visibility are resolved on every collection.
	text->text = "C";
	renderer.Collect();
	CHECK(renderer.DrawList().size() == 2);
	parent.Add<Hidden>();
	renderer.Collect();
	CHECK(renderer.DrawList().size() == 1);
	parent.Remove<Hidden>();
	text->font = texture->Id(); // wrong resource type
	renderer.Collect();
	CHECK(renderer.DrawList().size() == 1);
	text->font = f.font->Id();
	f.font->Unload(f.resources.Context());
	renderer.Collect();
	CHECK(renderer.DrawList().size() == 1);
}

TEST_CASE("Text2D: reflection and scene round trip preserve text and font resources") {
	Fixture source;
	World world;
	Scene scene = CreateScene(world, "TextScene");
	Entity entity = editor::SpawnChild(scene.Root(), editor::EntityArchetype::Text2D);
	Text2D value;
	value.font = source.font->Id();
	value.text = "Hello\nWorld: \"quotes\"";
	value.fontSize = 48.f;
	value.wrapWidth = 300.f;
	value.lineSpacing = 1.2f;
	value.offset = {3.f, -4.f};
	value.horizontalAlignment = TextHorizontalAlignment::Right;
	value.verticalAlignment = TextVerticalAlignment::Top;
	value.modulate = Color(0.2f, 0.3f, 0.4f, 0.5f);
	entity.Add(value);
	scene.Own(source.resources, value.font);
	const auto type = world.FindComponent("Text2D");
	REQUIRE(type.IsValid());
	REQUIRE(type.FindField("font") != nullptr);
	CHECK(type.FindField("font")->resourceType == "Font");
	CHECK(type.Fields().size() == 9);
	Archive save = Archive::Save();
	REQUIRE(SaveScene(save, scene, source.resources));
	Fixture target;
	World restored;
	Archive load = Archive::Load(save.ToString());
	Scene loaded = LoadScene(load, restored, target.resources);
	REQUIRE(loaded.IsValid());
	CHECK_FALSE(load.HasErrors());
	Entity label = restored.Find("TextScene/Text2D");
	REQUIRE(label.IsAlive());
	const auto *copy = label.Get<Text2D>();
	REQUIRE(copy != nullptr);
	CHECK(copy->font == value.font);
	CHECK(copy->text == value.text);
	CHECK(copy->fontSize == value.fontSize);
	CHECK(copy->wrapWidth == value.wrapWidth);
	CHECK(copy->lineSpacing == value.lineSpacing);
	CHECK(copy->offset == value.offset);
	CHECK(copy->modulate.a == value.modulate.a);
	CHECK(copy->horizontalAlignment == value.horizontalAlignment);
	CHECK(copy->verticalAlignment == value.verticalAlignment);
	REQUIRE(target.resources.Get<Font>(copy->font) != nullptr);
	CHECK(target.resources.Get<Font>(copy->font)->IsLoaded());
}

TEST_CASE("Text2D: Lua can create and edit native text fields") {
	World world;
	ScriptServer scripts(world);
	REQUIRE(scripts.DoString(R"(
		local e = world:create_entity("Label")
		e:add("Transform2D")
		e:add("Text2D")
		local t = e:get("Text2D")
		t.text = "Score: 42\nReady"
		t.fontSize = 40
		t.wrapWidth = 200
		t.horizontalAlignment = "Center"
		t.verticalAlignment = "Top"
	)"));
	const auto *text = world.Find("Label").Get<Text2D>();
	REQUIRE(text != nullptr);
	CHECK(text->text == "Score: 42\nReady");
	CHECK(text->fontSize == 40);
	CHECK(text->wrapWidth == 200);
	CHECK(text->horizontalAlignment == TextHorizontalAlignment::Center);
	CHECK(text->verticalAlignment == TextVerticalAlignment::Top);
}
