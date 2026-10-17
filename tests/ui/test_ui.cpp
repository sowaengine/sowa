#include <cstring>
#include <doctest/doctest.h>

#include "core/filesystem/file_system.hpp"
#include "ecs/components/camera2d.hpp"
#include "ecs/components/nine_patch_rect.hpp"
#include "ecs/components/transform2d.hpp"
#include "ecs/entity_pick_map.hpp"
#include "ecs/world.hpp"
#include "editor/hierarchy.hpp"
#include "editor/scene_commands.hpp"
#include "render/renderer2d.hpp"
#include "render/stub_graphics_device.hpp"
#include "resource/font.hpp"
#include "resource/image_texture.hpp"
#include "scene/scene_serializer.hpp"
#include "script/lua_ui.hpp"
#include "script/script_server.hpp"
#include "ui/drawing.hpp"
#include "ui/geometry.hpp"
#include "ui/ui_system.hpp"

using namespace sowa;

namespace {
struct Device : StubGraphicsDevice {
	std::vector<u32> draws;
	std::vector<glm::mat4> projections;

	void SetUniformMat4(const char *name, const glm::mat4 &matrix) override {
		if (std::string_view(name) == "uProj") {
			projections.push_back(matrix);
		}
	}

	std::vector<std::vector<u8>> uploads;
	PipelineDesc pipeline;
	u32 buffers = 0;

	PipelineHandle CreatePipeline(const PipelineDesc &desc) override {
		pipeline = desc;
		return {1, 1};
	}

	BufferHandle CreateBuffer(BufferType, BufferUsage, u32, const void *) override {
		return {++buffers, 1};
	}

	void UpdateBuffer(BufferHandle, const void *data, u32 size, u32) override {
		const auto *p = static_cast<const u8 *>(data);
		uploads.emplace_back(p, p + size);
	}

	void DrawIndexed(BufferHandle, BufferHandle, u32 count) override {
		draws.push_back(count);
	}
};

struct Fixture {
	Device device;
	FileSystem fs;
	ResourceRegistry resources;
	World world;
	UISystem ui;
	Input input;
	Scene scene;
	Entity canvas;

	Fixture() {
		fs.MountFolder("res", SOWA_EXAMPLE_DIR);
		resources.SetContext({&device, &fs, &resources});
		resources.RegisterType<Font>("Font")
			.Field("path", &Font::path)
			.Field("pixelSize", &Font::pixelSize);
		RegisterUITheme(resources);
		auto font = resources.Create<Font>();
		resources.Reload(font->Id());
		ui.Init(world, resources);
		ui.SetDefaultFont(font->Id());
		scene = CreateScene(world, "UI");
		canvas = CreateUI(scene.Root(), "UICanvas");
		canvas.Get<UICanvas>()->scaleMode = UIScaleMode::Viewport;
		canvas.Get<UICanvas>()->space = UISpace::Screen;
		SetUIAnchorPreset(*canvas.Get<UIRect>(), "Stretch");
		input.SetViewportMapping({0, 0, 800, 600}, {800, 600}, {800, 600}, {800, 600});
		input.Events().append([this](const InputEvent &e) { ui.QueueInput(e); });
	}

	void layout() {
		ui.Layout({800, 600});
	}

	void tick() {
		layout();
		ui.HandleInput(input, 1.f / 60);
		layout();
		input.Update();
	}

	void click(f32 x, f32 y) {
		input.HandleCursorPos(x, y);
		input.HandleMouseButton(0, KeyAction::Press);
		input.HandleMouseButton(0, KeyAction::Release);
		tick();
	}

	void key(Key key, i32 mods = 0) {
		input.HandleKey(key, KeyAction::Press, mods);
		input.HandleKey(key, KeyAction::Release, mods);
		tick();
	}

	Entity add(const std::string &kind, Vector2 p = {0, 0}, Vector2 size = {160, 40}) {
		Entity e = CreateUI(canvas, kind);
		e.Get<UIRect>()->position = p;
		e.Get<UIRect>()->size = size;
		return e;
	}
};
} // namespace

TEST_CASE("UI: anchors stretch and fit-reference sizing resolve across viewport changes") {
	Fixture f;
	auto e = f.add("UIButton", {-20, -30}, {100, 40});
	SetUIAnchorPreset(*e.Get<UIRect>(), "BottomRight");
	e.Get<UIRect>()->position = {-20, -30};
	f.layout();
	CHECK(e.Get<UIRect>()->resolved.x == 680);
	CHECK(e.Get<UIRect>()->resolved.y == 530);
	f.ui.Layout({1000, 700});
	CHECK(e.Get<UIRect>()->resolved.x == 880);
	CHECK(e.Get<UIRect>()->resolved.y == 630);
	SetUIAnchorPreset(*e.Get<UIRect>(), "Stretch");
	e.Get<UIRect>()->size = {-40, -60};
	e.Get<UIRect>()->position = {20, 30};
	f.layout();
	CHECK(e.Get<UIRect>()->resolved.w == 760);
	CHECK(e.Get<UIRect>()->resolved.h == 540);
	f.canvas.Get<UICanvas>()->scaleMode = UIScaleMode::FitReference;
	f.canvas.Get<UICanvas>()->referenceSize = {400, 300};
	e.Get<UIRect>()->anchorMax = {0, 0};
	e.Get<UIRect>()->size = {100, 40};
	f.layout();
	CHECK(e.Get<UIRect>()->resolved.w == 200);
}

TEST_CASE("UI: vertical containers measure, arrange, grow, constrain and skip hidden children") {
	Fixture f;
	auto column = f.add("UIColumn", {10, 20}, {300, 300});
	auto *layout = column.Get<UILayout>();
	layout->padding = {10, 10};
	layout->spacing = {0, 5};
	auto a = CreateUI(column, "UIButton"), b = CreateUI(column, "UIButton");
	a.Get<UIRect>()->size = {100, 30};
	b.Get<UIRect>()->size = {100, 30};
	b.Get<UIRect>()->grow = 1;
	f.layout();
	CHECK(a.Get<UIRect>()->resolved.x == 20);
	CHECK(a.Get<UIRect>()->resolved.w == 280);
	CHECK(b.Get<UIRect>()->resolved.y == 65);
	CHECK(b.Get<UIRect>()->resolved.h == 245);
	a.SetVisible(false);
	f.layout();
	CHECK(b.Get<UIRect>()->resolved.y == 30);
	CHECK(b.Get<UIRect>()->resolved.h == 280);
	b.Get<UIRect>()->maxSize.x = 120;
	f.layout();
	CHECK(b.Get<UIRect>()->resolved.w == 120);
}

TEST_CASE("UI: grid and flow containers arrange children without creation-order assumptions") {
	Fixture f;
	auto group = f.add("UIGrid", {0, 0}, {220, 180});
	group.Get<UILayout>()->padding = {0, 0};
	group.Get<UILayout>()->spacing = {10, 10};
	group.Get<UILayout>()->columns = 2;
	auto a = CreateUI(group, "UIButton"), b = CreateUI(group, "UIButton"),
		 c = CreateUI(group, "UIButton");
	for (auto e : {a, b, c}) {
		e.Get<UIRect>()->size = {100, 30};
	}
	f.layout();
	CHECK(a.Get<UIRect>()->resolved.x == 0);
	CHECK(b.Get<UIRect>()->resolved.x == 115);
	CHECK(c.Get<UIRect>()->resolved.y == 40);
	group.Get<UILayout>()->mode = UILayoutMode::Flow;
	f.layout();
	CHECK(b.Get<UIRect>()->resolved.x == 110);
	CHECK(c.Get<UIRect>()->resolved.y == 40);
}

TEST_CASE("UI: content-sized text measures wrapped height after container width") {
	Fixture f;
	auto group = f.add("UIColumn", {0, 0}, {180, 300});
	group.Get<UILayout>()->padding = {0, 0};
	auto label = CreateUI(group, "UILabel");
	label.Get<UILabel>()->text =
		"A paragraph that wraps across several lines in a narrow container.";
	label.Get<UILabel>()->wrap = true;
	label.Get<UIRect>()->fitContentY = true;
	auto button = CreateUI(group, "UIButton");
	f.layout();
	const f32 height = label.Get<UIRect>()->resolved.h;
	CHECK(height > 40);
	CHECK(button.Get<UIRect>()->resolved.y > height);
	group.Get<UIRect>()->size.x = 400;
	f.layout();
	CHECK(label.Get<UIRect>()->resolved.h < height);
}

TEST_CASE("UI: clipped scrolling content cannot intercept clicks outside its viewport") {
	Fixture f;
	auto scroll = f.add("UIScrollView", {20, 20}, {200, 100});
	scroll.Get<UILayout>()->padding = {0, 0};
	scroll.Get<UILayout>()->spacing = {0, 0};
	auto a = CreateUI(scroll, "UIButton"), b = CreateUI(scroll, "UIButton"),
		 c = CreateUI(scroll, "UIButton");
	for (auto e : {a, b, c}) {
		e.Get<UIRect>()->size = {200, 50};
	}
	f.layout();
	CHECK(scroll.Get<UIScrollView>()->contentSize.y == 150);
	CHECK_FALSE(f.ui.HitTest({50, 145}).IsAlive());
	f.ui.ScrollTo(scroll, {0, 999});
	f.layout();
	CHECK(scroll.Get<UIScrollView>()->offset.y == 50);
	CHECK(c.Get<UIRect>()->resolved.y == 70);
	CHECK(f.ui.HitTest({50, 95}) == c);
}

TEST_CASE("UI: capture cancels outside releases and controls emit bubbling events") {
	Fixture f;
	auto button = f.add("UIButton", {20, 20});
	int clicks = 0, parents = 0;
	f.ui.On(button, "clicked", [&](UIEvent &) { ++clicks; });
	f.ui.On(f.canvas, "clicked", [&](UIEvent &e) {
		CHECK(e.target == button);
		++parents;
	});
	f.click(40, 40);
	CHECK(clicks == 1);
	CHECK(parents == 1);
	f.input.HandleCursorPos(40, 40);
	f.input.HandleMouseButton(0, KeyAction::Press);
	f.tick();
	f.input.HandleCursorPos(700, 500);
	f.input.HandleMouseButton(0, KeyAction::Release);
	f.tick();
	CHECK(clicks == 1);
	f.ui.On(button, "clicked", [](UIEvent &e) { e.handled = true; });
	f.click(40, 40);
	CHECK(clicks == 2);
	CHECK(parents == 1);
}

TEST_CASE("UI: destroyed controls and callbacks are safe during event dispatch") {
	Fixture f;
	auto button = f.add("UIButton");
	int calls = 0;
	f.ui.On(button, "clicked", [&](UIEvent &) {
		++calls;
		button.Destroy();
	});
	f.click(20, 20);
	CHECK(calls == 1);
	CHECK_FALSE(button.IsAlive());
	f.tick();
	CHECK_FALSE(f.ui.Focused().IsAlive());
}

TEST_CASE("UI: keyboard traversal skips disabled controls and drives toggles and sliders") {
	Fixture f;
	auto a = f.add("UIButton"), b = f.add("UIToggle", {0, 60}), c = f.add("UISlider", {0, 120});
	a.Get<UIControl>()->disabled = true;
	f.layout();
	f.key(Key::Tab);
	CHECK(f.ui.Focused() == b);
	f.key(Key::Space);
	CHECK(b.Get<UIToggle>()->checked);
	f.key(Key::Tab);
	CHECK(f.ui.Focused() == c);
	f.key(Key::Right);
	CHECK(c.Get<UISlider>()->value == doctest::Approx(.51f));
	f.key(Key::End);
	CHECK(c.Get<UISlider>()->value == 1);
	f.key(Key::Tab, 1);
	CHECK(f.ui.Focused() == b);
}

TEST_CASE("UI: slider capture clamps and quantizes pointer values outside its rectangle") {
	Fixture f;
	auto slider = f.add("UISlider", {20, 20}, {200, 30});
	slider.Get<UISlider>()->step = .1f;
	f.input.HandleCursorPos(67, 30);
	f.input.HandleMouseButton(0, KeyAction::Press);
	f.tick();
	CHECK(slider.Get<UISlider>()->value == doctest::Approx(.2));
	f.input.HandleCursorPos(700, 30);
	f.tick();
	CHECK(slider.Get<UISlider>()->value == 1);
}

TEST_CASE("UI: text entry supports selection, clipboard, max length and read-only mode") {
	Fixture f;
	auto text = f.add("UITextInput", {20, 20}, {240, 40});
	std::string clipboard;
	f.ui.SetClipboard([&] { return clipboard; }, [&](const std::string &s) { clipboard = s; });
	f.click(30, 30);
	f.input.HandleChar('a');
	f.input.HandleChar('b');
	f.input.HandleChar('c');
	f.tick();
	CHECK(text.Get<UITextInput>()->text == "abc");
	f.key(Key::A, 2);
	f.key(Key::C, 2);
	CHECK(clipboard == "abc");
	f.input.HandleChar('Z');
	f.tick();
	CHECK(text.Get<UITextInput>()->text == "Z");
	clipboard = "hello\nworld";
	text.Get<UITextInput>()->maxLength = 5;
	f.key(Key::A, 2);
	f.key(Key::V, 2);
	CHECK(text.Get<UITextInput>()->text == "hello");
	text.Get<UITextInput>()->readOnly = true;
	f.key(Key::Backspace);
	CHECK(text.Get<UITextInput>()->text == "hello");
}

TEST_CASE("UI: modal popups trap focus and block background then restore focus") {
	Fixture f;
	auto button = f.add("UIButton", {10, 10});
	f.layout();
	REQUIRE(f.ui.Focus(button));
	auto popup = f.add("UIModal", {200, 100}, {300, 200});
	SetUIAnchorPreset(*popup.Get<UIRect>(), "TopLeft");
	popup.Get<UIRect>()->position = {200, 100};
	auto inside = CreateUI(popup, "UIButton");
	f.ui.Open(popup);
	f.tick();
	CHECK(f.ui.Focused() == inside);
	CHECK_FALSE(f.ui.HitTest({20, 20}).IsAlive());
	f.key(Key::Escape);
	f.tick();
	CHECK_FALSE(popup.Get<UIPopup>()->open);
	CHECK(f.ui.Focused() == button);
}

TEST_CASE("UI: dropdown selection and tabs expose their selected values") {
	Fixture f;
	auto dropdown = f.add("UIDropdown", {20, 20}, {160, 40});
	f.ui.SetItems(dropdown, {"One", "Two", "Three"});
	f.click(40, 30);
	CHECK(dropdown.Get<UIDropdown>()->open);
	f.click(40, 110);
	CHECK(dropdown.Get<UIDropdown>()->selected == 1);
	CHECK_FALSE(dropdown.Get<UIDropdown>()->open);
	auto tabs = f.add("UITabs", {300, 20}, {300, 200});
	auto a = CreateUI(tabs, "UIButton"), b = CreateUI(tabs, "UIButton");
	f.click(540, 30);
	CHECK(tabs.Get<UITabs>()->selected == 1);
	f.layout();
	CHECK(f.ui.HitTest({320, 80}) == b);
}

TEST_CASE("UI: theme inheritance and scene serialization preserve native fields and resources") {
	Fixture f;
	auto theme = f.resources.Create<UITheme>();
	theme->fontSize = 30;
	UIThemeRule rule;
	rule.role = "Button";
	rule.variant = "Danger";
	rule.appearance = *theme;
	rule.appearance.background = {1, 0, 0, 1};
	theme->rules.push_back(rule);
	CHECK(theme->Resolve("Button", "Danger").background.r == 1);
	CHECK(theme->Resolve("Label", "").fontSize == 30);
	f.canvas.Get<UICanvas>()->theme = theme->Id();
	f.scene.Own(f.resources, theme->Id());
	auto button = f.add("UIButton");
	button.Get<UIButton>()->text = "Save me";
	button.Add(UIStyle{{}, "Danger"});
	Archive save = Archive::Save();
	REQUIRE(SaveScene(save, f.scene, f.resources));
	Fixture target;
	World restored;
	Archive load = Archive::Load(save.ToString());
	auto scene = LoadScene(load, restored, target.resources);
	REQUIRE(scene.IsValid());
	CHECK_FALSE(load.HasErrors());
	auto copy = restored.Find("UI/UICanvas/UIButton");
	REQUIRE(copy.IsAlive());
	CHECK(copy.Get<UIButton>()->text == "Save me");
	CHECK(copy.Get<UIStyle>()->variant == "Danger");
	auto restoredTheme = target.resources.Get<UITheme>(theme->Id());
	REQUIRE(restoredTheme);
	CHECK(restoredTheme->rules.size() == 1);
}

TEST_CASE("UI: Lua presets and event handlers use the same native controls") {
	Fixture f;
	ScriptServer scripts(f.world);
	BindUI(scripts, f.ui);
	scripts.SetEntity("canvas", f.canvas);
	REQUIRE(scripts.DoString(
		R"(button=ui.create(canvas,"UIButton","Save");button:get("UIButton").text="Save";clicks=0;ui.on(button,"clicked",function(event) clicks=clicks+1; assert(event.target==button) end))"));
	f.click(20, 20);
	REQUIRE(scripts.DoString("assert(clicks==1)"));
	f.ui.Reset();
}

TEST_CASE("UI: renderer keeps distinct nested clips within a single batch and culls empty clips") {
	Device device;
	FileSystem fs;
	fs.MountFolder("res", SOWA_EXAMPLE_DIR);
	Renderer2D renderer;
	REQUIRE(renderer.Init(device, fs));
	renderer.Begin();
	renderer.PushClip({10, 10, 100, 100});
	renderer.PushClip({50, 0, 100, 50});
	CHECK(renderer.CurrentClip().w == 60);
	CHECK(renderer.CurrentClip().h == 40);
	renderer.DrawQuad(0, 0, 200, 200, Color{});
	renderer.PopClip();
	renderer.DrawQuad(0, 0, 200, 200, Color{});
	renderer.PushClip({200, 200, 10, 10});
	renderer.DrawQuad(0, 0, 400, 400, Color{});
	renderer.PopClip();
	renderer.PopClip();
	renderer.DrawQuad(300, 300, 20, 20, Color{});
	renderer.End();
	REQUIRE(device.draws.size() == 1);
	CHECK(device.draws[0] == 18);
	REQUIRE(device.uploads.size() == 1);
	// 10 floats + token + 4 clip floats per vertex, matching the pipeline layout.
	const auto &bytes = device.uploads[0];
	REQUIRE(bytes.size() == 12 * 60);
	float first[15];
	std::memcpy(first, bytes.data(), 60);
	CHECK(first[11] == 50);
	CHECK(first[12] == 10);
	CHECK(first[13] == 110);
	CHECK(first[14] == 50);
	std::memcpy(first, bytes.data() + 4 * 60, 60);
	CHECK(first[11] == 10);
	CHECK(first[13] == 110);
	renderer.Begin();
	CHECK(renderer.CurrentClip().w > 1e10f);
}

TEST_CASE("UI: example scene draws and its Lua settings and theme actions work") {
	Fixture f;
	ScriptServer scripts(f.world);
	BindUI(scripts, f.ui);
	REQUIRE(scripts.DoFile(f.fs, "res://scripts/ui_demo.lua"));
	Scene demo = LoadSceneFromFile(f.fs, "res://scenes/ui.scene", f.world, f.resources);
	REQUIRE(demo.IsValid());
	CHECK(demo.Root().GetName() == "UIDemo");
	demo.Root().Find("Canvas").Get<UICanvas>()->space = UISpace::Screen;
	demo.Root().Find("Canvas").Get<Transform2D>()->position = {0, 0};
	f.scene.SetEnabled(false);
	scripts.SyncLifecycles();
	f.world.Progress(.016f);
	f.layout();
	Entity canvas = demo.Root().Find("Canvas");
	REQUIRE(canvas.IsAlive());
	Entity button = canvas.Find("Content/Settings/Actions/Save");
	REQUIRE(button.IsAlive());
	const auto r = button.Get<UIRect>()->resolved;
	f.click(r.x + r.w / 2, r.y + r.h / 2);
	CHECK(canvas.Find("Content/Settings/Status").Get<UILabel>()->text ==
		  "Saved settings for Player.");
	const auto t = canvas.Find("Theme").Get<UIRect>()->resolved;
	f.click(t.x + 20, t.y + 10);
	CHECK(canvas.Get<UICanvas>()->theme.value == 990103);
	Renderer2D renderer;
	REQUIRE(renderer.Init(f.device, f.fs));
	EntityPickMap picks;
	f.ui.Draw(renderer, picks);
	CHECK_FALSE(f.device.draws.empty());
	f.ui.Reset();
}

TEST_CASE("UI: presets can be created in deferred Lua gameplay systems") {
	Fixture f;
	ScriptServer scripts(f.world);
	BindUI(scripts, f.ui);
	scripts.SetEntity("canvas", f.canvas);
	REQUIRE(scripts.DoString(
		R"(world:system("MakeUI"):each(function() if not made then made=ui.create(canvas,"UIButton","Dynamic");made:get("UIButton").text="Created safely" end end))"));
	f.world.Progress(.016f);
	auto dynamic = f.canvas.Find("Dynamic");
	REQUIRE(dynamic.IsAlive());
	CHECK(dynamic.Get<UIButton>()->text == "Created safely");
	f.ui.Reset();
}

TEST_CASE("UI: scrollbars intercept their contents but remain below modal overlays") {
	Fixture f;
	auto scroll = f.add("UIScrollView", {20, 20}, {200, 100});
	scroll.Get<UILayout>()->padding = {0, 0};
	scroll.Get<UILayout>()->spacing = {0, 0};
	auto content = CreateUI(scroll, "UIButton");
	content.Get<UIRect>()->size = {200, 400};
	f.layout();
	CHECK(f.ui.HitTest({215, 40}) == scroll);
	f.input.HandleCursorPos(215, 30);
	f.input.HandleMouseButton(0, KeyAction::Press);
	f.tick();
	f.input.HandleCursorPos(215, 100);
	f.tick();
	CHECK(scroll.Get<UIScrollView>()->offset.y > 200);
	f.input.HandleMouseButton(0, KeyAction::Release);
	f.tick();
	auto modal = f.add("UIModal");
	f.ui.Open(modal);
	f.tick();
	CHECK_FALSE(f.ui.HitTest({215, 40}).IsAlive());
	Renderer2D renderer;
	REQUIRE(renderer.Init(f.device, f.fs));
	EntityPickMap picks;
	f.ui.Draw(renderer, picks);
	bool seenModal = false;
	size_t scrollQuads = 0;
	for (const auto &bytes : f.device.uploads) {
		for (size_t offset = 0; offset + 60 <= bytes.size(); offset += 4 * 60) {
			u32 token = 0;
			std::memcpy(&token, bytes.data() + offset + 40, sizeof(token));
			const Entity entity = picks.Resolve(token);
			if (entity == modal) {
				seenModal = true;
			}
			if (entity == scroll) {
				CHECK_FALSE(seenModal);
				++scrollQuads;
			}
		}
	}
	CHECK(seenModal);
	CHECK(scrollQuads >= 2);
}

TEST_CASE("UI: nested dialogs restore each level of keyboard focus") {
	Fixture f;
	auto base = f.add("UIButton");
	f.layout();
	REQUIRE(f.ui.Focus(base));
	auto outer = f.add("UIModal"), outerButton = CreateUI(outer, "UIButton");
	auto inner = CreateUI(outer, "UIModal"), innerButton = CreateUI(inner, "UIButton");
	f.ui.Open(outer);
	f.tick();
	CHECK(f.ui.Focused() == outerButton);
	f.ui.Open(inner);
	f.tick();
	CHECK(f.ui.Focused() == innerButton);
	f.ui.Open(inner, false);
	f.tick();
	CHECK(f.ui.Focused() == outerButton);
	f.ui.Open(outer, false);
	f.tick();
	CHECK(f.ui.Focused() == base);
}

TEST_CASE("UI: capture survives a destroying callback and wheel handlers can consume scrolling") {
	Fixture f;
	auto button = f.add("UIButton");
	f.ui.On(button, "clicked", [&](UIEvent &) { button.Destroy(); });
	f.input.HandleCursorPos(20, 20);
	f.input.HandleMouseButton(0, KeyAction::Press);
	f.input.HandleMouseButton(0, KeyAction::Release);
	f.layout();
	f.ui.HandleInput(f.input, .016f);
	CHECK(f.ui.CapturesPointer());
	CHECK_FALSE(f.input.IsKeyJustReleased(Key::MouseLeft));
	f.input.Update();
	auto scroll = f.add("UIScrollView");
	auto content = CreateUI(scroll, "UIButton");
	content.Get<UIRect>()->size = {100, 600};
	f.ui.On(scroll, "wheel", [](UIEvent &e) {
		CHECK(e.delta.y == -1);
		e.handled = true;
	});
	f.input.HandleScroll(0, -1);
	f.layout();
	f.ui.HandleInput(f.input, .016f);
	CHECK(scroll.Get<UIScrollView>()->offset.y == 0);
	CHECK(f.input.ScrollDelta().y == 0);
	f.ui.HandleInput(f.input, .016f, false);
	CHECK_FALSE(scroll.Get<UIControl>()->hovered);
	CHECK_FALSE(scroll.Get<UIControl>()->pressed);
}

TEST_CASE("UI: a disabled-on-press button cannot click on release") {
	Fixture f;
	auto button = f.add("UIButton");
	int clicks = 0;
	f.ui.On(button, "pressed", [&](UIEvent &) { button.Get<UIControl>()->disabled = true; });
	f.ui.On(button, "clicked", [&](UIEvent &) { ++clicks; });
	f.click(20, 20);
	CHECK(clicks == 0);
	CHECK_FALSE(f.ui.Focused().IsAlive());
}

TEST_CASE("UI: nine-slice geometry preserves independent borders and clips tiled regions") {
	Fixture f;
	f.resources.RegisterType<ImageTexture>("ImageTexture");
	auto texture = f.resources.Create<ImageTexture>();
	texture->Adopt({&f.device, &f.fs, &f.resources}, {77, 1}, 64, 64);
	UIImage image;
	image.texture = texture->Id();
	image.regionPosition = {8, 8};
	image.regionSize = {32, 32};
	image.mode = UIImageMode::NineSlice;
	image.borderMin = {4, 6};
	image.borderMax = {8, 10};
	Renderer2D renderer;
	REQUIRE(renderer.Init(f.device, f.fs));
	renderer.Begin();
	DrawUIImage(renderer, f.resources, image, {20, 30, 100, 80}, 600, 1, Color{}, 42);
	renderer.End();
	REQUIRE(f.device.draws.size() == 1);
	CHECK(f.device.draws[0] == 54);
	REQUIRE(f.device.uploads.size() == 1);
	const auto &bytes = f.device.uploads[0];
	REQUIRE(bytes.size() == 9 * 4 * 60);
	auto vertex = [&](size_t quad, size_t corner) {
		float values[15];
		std::memcpy(values, bytes.data() + (quad * 4 + corner) * 60, 60);
		return std::vector<float>(values, values + 15);
	};
	const auto a = vertex(0, 0), b = vertex(0, 2);
	CHECK(a[0] == 20);
	CHECK(b[0] == 24);
	CHECK(std::abs(b[1] - a[1]) == 6);
	CHECK(a[3] == doctest::Approx(8.f / 64));
	CHECK(std::abs(b[3] - a[3]) == doctest::Approx(4.f / 64));
	for (size_t i = 0; i < 9; ++i) {
		CHECK(vertex(i, 0)[11] == 20);
		CHECK(vertex(i, 0)[13] == 120);
	}
	f.device.draws.clear();
	f.device.uploads.clear();
	image.mode = UIImageMode::Tile;
	image.regionSize = {16, 16};
	renderer.Begin();
	renderer.PushClip({0, 590, 10, 10});
	DrawUIImage(renderer, f.resources, image, {0, 0, 1000000, 1000000}, 600, 1, Color{}, 0);
	renderer.End();
	REQUIRE(f.device.draws.size() == 1);
	CHECK(f.device.draws[0] == 6);
}

TEST_CASE("UI: editor rectangle presets undo and redo as one operation") {
	Fixture f;
	auto button = f.add("UIButton", {30, 40}, {120, 50});
	std::vector<editor::SceneDocumentSaveState> states;
	editor::UpdateSceneDocumentStates(states, {f.scene});
	const UIRect before = *button.Get<UIRect>();
	SetUIAnchorPreset(*button.Get<UIRect>(), "Stretch");
	REQUIRE(editor::RecordUIRectEdit(states, f.scene, button, before, *button.Get<UIRect>()));
	REQUIRE(editor::UndoSceneDocument(states, f.scene));
	CHECK(button.Get<UIRect>()->position == Vector2(30, 40));
	CHECK(button.Get<UIRect>()->size == Vector2(120, 50));
	REQUIRE(editor::RedoSceneDocument(states, f.scene));
	CHECK(button.Get<UIRect>()->anchorMax == Vector2(1, 1));
	CHECK(button.Get<UIRect>()->size == Vector2(0, 0));
}

TEST_CASE("UI: ensure-visible accounts for movement inside nested scrollviews") {
	Fixture f;
	auto outer = f.add("UIScrollView", {0, 100}, {200, 100});
	outer.Get<UILayout>()->mode = UILayoutMode::Free;
	outer.Get<UILayout>()->padding = {0, 0};
	auto inner = CreateUI(outer, "UIScrollView");
	inner.Get<UILayout>()->mode = UILayoutMode::Free;
	inner.Get<UILayout>()->padding = {0, 0};
	inner.Get<UIRect>()->position = {0, 100};
	inner.Get<UIRect>()->size = {200, 100};
	auto target = CreateUI(inner, "UIButton");
	target.Get<UIRect>()->position = {0, 250};
	target.Get<UIRect>()->size = {100, 40};
	auto end = CreateUI(outer, "UIRect");
	end.Get<UIRect>()->position = {0, 800};
	f.layout();
	f.ui.EnsureVisible(target);
	f.layout();
	CHECK(inner.Get<UIScrollView>()->offset.y == 190);
	CHECK(outer.Get<UIScrollView>()->offset.y == 100);
	CHECK(f.ui.HitTest({20, 170}) == target);
}

TEST_CASE("UI: UTF-8 text limits and deletion preserve complete codepoints") {
	Fixture f;
	auto field = f.add("UITextInput");
	field.Get<UITextInput>()->maxLength = 2;
	std::string clipboard = "unchanged";
	f.ui.SetClipboard([&] { return clipboard; },
					  [&](const std::string &text) { clipboard = text; });
	f.click(20, 20);
	f.input.HandleChar(0xe9);
	f.input.HandleChar(0x1f600);
	f.input.HandleChar('x');
	f.tick();
	CHECK(field.Get<UITextInput>()->text == "\xc3\xa9\xf0\x9f\x98\x80");
	f.key(Key::Backspace);
	CHECK(field.Get<UITextInput>()->text == "\xc3\xa9");
	field.Get<UITextInput>()->password = true;
	f.key(Key::A, 2);
	f.key(Key::X, 2);
	CHECK(clipboard == "unchanged");
	CHECK(field.Get<UITextInput>()->text == "\xc3\xa9");
}

TEST_CASE("UI: world canvases honor size, rectangle position and inherited 2D transforms") {
	Fixture f;
	auto &canvas = *f.canvas.Get<UICanvas>();
	canvas.space = UISpace::World;
	auto &rect = *f.canvas.Get<UIRect>();
	SetUIAnchorPreset(rect, "TopLeft");
	rect.size = {200, 100};
	f.canvas.Get<Transform2D>()->position = {100, 400};
	auto button = f.add("UIButton", {20, 10}, {80, 30});
	f.layout();
	CHECK(f.canvas.Get<UIRect>()->resolved.w == 200);
	CHECK(f.canvas.Get<UIRect>()->resolved.h == 100);
	CHECK(button.Get<UIRect>()->viewportCorners[0].x == doctest::Approx(120));
	CHECK(button.Get<UIRect>()->viewportCorners[0].y == doctest::Approx(210));
	CHECK(f.ui.HitTest({130, 220}) == button);
	CHECK_FALSE(f.ui.HitTest({30, 20}).IsAlive());

	f.canvas.Get<UIRect>()->position = {30, 40};
	f.scene.Root().Add(Transform2D{{50, 20}});
	f.layout();
	CHECK(button.Get<UIRect>()->viewportCorners[0].x == doctest::Approx(200));
	CHECK(button.Get<UIRect>()->viewportCorners[0].y == doctest::Approx(230));
	CHECK(f.ui.HitTest({210, 240}) == button);
	CHECK_FALSE(f.ui.HitTest({130, 220}).IsAlive());
}

TEST_CASE("UI: rotated camera and canvas share rendering, clipping and pointer coordinates") {
	Fixture f;
	f.canvas.Get<UICanvas>()->space = UISpace::World;
	SetUIAnchorPreset(*f.canvas.Get<UIRect>(), "TopLeft");
	f.canvas.Get<UIRect>()->size = {160, 100};
	auto &transform = *f.canvas.Get<Transform2D>();
	transform.position = {200, 400};
	transform.rotation = 25;
	transform.scale = {1.5f, 1.5f};
	auto button = f.add("UIButton", {20, 20}, {80, 30});
	auto camera = f.scene.Root().CreateChild("Camera");
	camera.Add(Transform2D{{200, 350}, -20, {2, 2}});
	camera.Add(Camera2D{});
	camera.Get<Camera2D>()->rotatable = true;
	const auto view = CalculateViewProjection(camera, {800, 600});
	f.ui.Layout({800, 600}, view);
	const auto &mapping = *f.canvas.Get<UICanvas>();
	const auto point = UITransformPoint(mapping.layoutToViewport, {40, 30});
	CHECK(f.ui.HitTest(point) == button);
	CHECK_FALSE(f.ui.HitTest(UITransformPoint(mapping.layoutToViewport, {190, 30})).IsAlive());
	int clicks = 0;
	f.ui.On(button, "clicked", [&](UIEvent &) { ++clicks; });
	f.input.HandleCursorPos(point.x, point.y);
	f.input.HandleMouseButton(0, KeyAction::Press);
	f.input.HandleMouseButton(0, KeyAction::Release);
	f.ui.HandleInput(f.input, .016f);
	CHECK(clicks == 1);

	Renderer2D renderer;
	REQUIRE(renderer.Init(f.device, f.fs));
	EntityPickMap picks;
	f.ui.Draw(renderer, picks);
	REQUIRE_FALSE(f.device.projections.empty());
	CHECK(f.device.projections.back() == mapping.projection);
	const auto drawPoint = glm::vec4(40, 600 - 30, 0, 1);
	const auto projected = f.device.projections.back() * drawPoint;
	CHECK((projected.x / projected.w + 1) * 400 == doctest::Approx(point.x));
	CHECK((1 - projected.y / projected.w) * 300 == doctest::Approx(point.y));

	camera.Get<Transform2D>()->position.x += 100;
	f.ui.Layout({800, 600}, CalculateViewProjection(camera, {800, 600}));
	CHECK_FALSE(f.ui.HitTest(point).IsAlive());
}

TEST_CASE("UI: a screen-space HUD ignores the world camera but keeps authored bounds") {
	Fixture f;
	auto &rect = *f.canvas.Get<UIRect>();
	SetUIAnchorPreset(rect, "TopLeft");
	rect.position = {30, 40};
	rect.size = {200, 100};
	auto button = f.add("UIButton", {10, 20}, {80, 30});
	const auto unrelatedView = glm::mat4(2.f);
	f.ui.Layout({800, 600}, unrelatedView);
	CHECK(button.Get<UIRect>()->viewportCorners[0].x == doctest::Approx(40));
	CHECK(button.Get<UIRect>()->viewportCorners[0].y == doctest::Approx(60));
	CHECK(f.ui.HitTest({50, 70}) == button);
	f.canvas.Get<Transform2D>()->position.x = 50;
	f.ui.Layout({800, 600}, unrelatedView);
	CHECK(f.ui.HitTest({100, 70}) == button);
	CHECK_FALSE(f.ui.HitTest({50, 70}).IsAlive());
}

TEST_CASE("UI: slider drags use the transformed canvas coordinates") {
	Fixture f;
	f.canvas.Get<UICanvas>()->space = UISpace::World;
	SetUIAnchorPreset(*f.canvas.Get<UIRect>(), "TopLeft");
	f.canvas.Get<UIRect>()->size = {200, 100};
	f.canvas.Get<Transform2D>()->position = {200, 400};
	f.canvas.Get<Transform2D>()->rotation = 35;
	f.canvas.Get<Transform2D>()->scale = {2, 2};
	auto slider = f.add("UISlider", {20, 20}, {100, 20});
	f.layout();
	const auto point = UITransformPoint(f.canvas.Get<UICanvas>()->layoutToViewport, {45, 30});
	f.click(point.x, point.y);
	CHECK(slider.Get<UISlider>()->value == doctest::Approx(.25f));
}

TEST_CASE("UI: every resize corner moves independently and keeps its opposite corner fixed") {
	UIRect before;
	before.size = {200, 100};
	before.pivot = {.5f, .5f};
	before.position = {200, 150};
	before.resolved = {100, 100, 200, 100};
	const Vector2 corners[] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
	for (Vector2 corner : corners) {
		const auto after = ResizeUIRect(before, {20, 10}, corner, 1);
		const Vector2 change = after.size - before.size;
		const Vector2 origin =
			Vector2(100, 100) + (after.position - before.position) - change * .5f;
		const Vector2 opposite{1 - corner.x, 1 - corner.y};
		CHECK(origin.x + opposite.x * after.size.x == doctest::Approx(100 + opposite.x * 200));
		CHECK(origin.y + opposite.y * after.size.y == doctest::Approx(100 + opposite.y * 100));
		CHECK(origin.x + corner.x * after.size.x == doctest::Approx(100 + corner.x * 200 + 20));
		CHECK(origin.y + corner.y * after.size.y == doctest::Approx(100 + corner.y * 100 + 10));
	}
	before.minSize = {50, 20};
	const auto constrained = ResizeUIRect(before, {1000, 1000}, {0, 0}, 1);
	CHECK(constrained.size == Vector2(50, 20));
}

TEST_CASE("UI: containers preserve child position as an offset from its layout slot") {
	Fixture f;
	auto row = f.add("UIRow", {20, 30}, {300, 100});
	row.Get<UILayout>()->padding = {0, 0};
	auto button = CreateUI(row, "UIButton");
	button.Get<UIRect>()->position = {15, 25};
	f.layout();
	CHECK(button.Get<UIRect>()->resolved.x == 35);
	CHECK(button.Get<UIRect>()->resolved.y == 55);
}

TEST_CASE("UI: passive controls keep their colors on hover and input metadata adds no surface") {
	for (const std::string kind :
		 {"UIScrollView", "UITabs", "UIList", "UITextInput", "UISlider", "UIPanel"}) {
		Fixture f;
		auto control = f.add(kind, {20, 20}, {200, 100});
		Renderer2D renderer;
		REQUIRE(renderer.Init(f.device, f.fs));
		EntityPickMap picks;
		f.layout();
		f.ui.Draw(renderer, picks);
		const auto before = f.device.uploads;
		f.device.uploads.clear();
		f.input.HandleCursorPos(40, 40);
		f.tick();
		f.ui.Draw(renderer, picks);
		CHECK(f.device.uploads == before);
	}
	Fixture f;
	auto region = f.add("UIRect");
	region.Add<UIControl>();
	f.layout();
	CHECK(f.ui.HitTest({20, 20}) == region);
	Renderer2D renderer;
	REQUIRE(renderer.Init(f.device, f.fs));
	EntityPickMap picks;
	f.ui.Draw(renderer, picks);
	CHECK(f.device.draws.empty());
}

TEST_CASE("UI: resizing uses visible dimensions for content-fit and constrained rectangles") {
	UIRect before;
	before.size = {100, 40};
	before.fitContentX = true;
	before.anchorSpan = {30, 0};
	before.resolved = {20, 20, 230, 80};
	const auto resized = ResizeUIRect(before, {20, 10}, {1, 1}, 1);
	CHECK(resized.size.x + before.anchorSpan.x == 250);
	CHECK(resized.size.y == 90);
	CHECK_FALSE(resized.fitContentX);
}

TEST_CASE("UI: nine-patch preset draws at its rectangle with tint, clip and picking") {
	Fixture f;
	f.resources.RegisterType<ImageTexture>("ImageTexture");
	auto texture = f.resources.Create<ImageTexture>();
	texture->Adopt({&f.device, &f.fs, &f.resources}, {77, 1}, 64, 64);
	auto entity = f.add("UINinePatchRect", {20, 30}, {100, 80});
	REQUIRE(entity.Has<UINinePatchRect>());
	CHECK_FALSE(entity.Has<UIControl>());
	auto &image = *entity.Get<UINinePatchRect>();
	image.texture = texture->Id();
	image.paddingLeft = 4;
	image.paddingTop = 6;
	image.paddingRight = 8;
	image.paddingBottom = 10;
	image.drawCenter = false;
	image.modulate = {.2f, .4f, .6f, .8f};
	f.layout();
	Renderer2D renderer;
	REQUIRE(renderer.Init(f.device, f.fs));
	EntityPickMap picks;
	f.ui.Draw(renderer, picks);
	REQUIRE(f.device.draws.size() == 1);
	CHECK(f.device.draws[0] == 8 * 6);
	REQUIRE(f.device.uploads.size() == 1);
	const auto &bytes = f.device.uploads[0];
	REQUIRE(bytes.size() == 8 * 4 * 60);
	float vertex[15];
	std::memcpy(vertex, bytes.data(), 60);
	CHECK(vertex[0] == 20);
	CHECK(vertex[1] == 570);
	CHECK(vertex[5] == doctest::Approx(.2f));
	CHECK(vertex[8] == doctest::Approx(.8f));
	CHECK(vertex[11] == 20);
	CHECK(vertex[12] == 490);
	CHECK(vertex[13] == 120);
	CHECK(vertex[14] == 570);
	u32 token;
	std::memcpy(&token, bytes.data() + 10 * sizeof(float), sizeof(token));
	CHECK(token == picks.TokenFor(entity));

	entity.Get<UIRect>()->fitContentX = true;
	entity.Get<UIRect>()->fitContentY = true;
	f.layout();
	CHECK(entity.Get<UIRect>()->resolved.w == 12);
	CHECK(entity.Get<UIRect>()->resolved.h == 16);
}

TEST_CASE("UI: nine-patch components are available to Lua and survive scene serialization") {
	Fixture f;
	ScriptServer scripts(f.world);
	BindUI(scripts, f.ui);
	scripts.SetEntity("canvas", f.canvas);
	auto worldPanel = editor::SpawnChild(f.scene.Root(), editor::EntityArchetype::NinePatchRect);
	REQUIRE(worldPanel.Has<NinePatchRect>());
	REQUIRE(worldPanel.Has<Transform2D>());
	scripts.SetEntity("worldPanel", worldPanel);
	REQUIRE(scripts.DoString(R"(
		local uiPanel = ui.create(canvas, "UINinePatchRect", "Frame")
		for _, entity in ipairs({uiPanel, worldPanel}) do
			local name = entity == uiPanel and "UINinePatchRect" or "NinePatchRect"
			local panel = entity:get(name)
			panel.paddingLeft = 4
			panel.paddingTop = 6
			panel.paddingRight = 8
			panel.paddingBottom = 10
			panel.tileCenter = true
			panel.tileEdges = true
			panel.drawCenter = false
		end
		worldPanel:get("NinePatchRect").size = Vector2(120, 90)
		worldPanel:get("NinePatchRect").offset = Vector2(5, 7)
		worldPanel:get("NinePatchRect").centered = false
	)"));
	f.resources.RegisterType<ImageTexture>("ImageTexture").Field("path", &ImageTexture::path);
	auto texture = f.resources.Create<ImageTexture>();
	texture->path = "res://textures/spritesheet_tiles.png";
	f.scene.Own(f.resources, texture->Id());
	worldPanel.Get<NinePatchRect>()->texture = texture->Id();
	f.canvas.Find("Frame").Get<UINinePatchRect>()->texture = texture->Id();
	Archive save = Archive::Save();
	REQUIRE(SaveScene(save, f.scene, f.resources));
	World restored;
	Fixture target;
	target.resources.RegisterType<ImageTexture>("ImageTexture").Field("path", &ImageTexture::path);
	Archive load = Archive::Load(save.ToString());
	const auto scene = LoadScene(load, restored, target.resources);
	REQUIRE(scene.IsValid());
	CHECK_FALSE(load.HasErrors());
	const auto *worldCopy = scene.Root().Find("NinePatchRect").Get<NinePatchRect>();
	const auto *uiCopy = scene.Root().Find("UICanvas/Frame").Get<UINinePatchRect>();
	REQUIRE(worldCopy);
	REQUIRE(uiCopy);
	auto check = [&](const auto &panel) {
		CHECK(panel.texture == texture->Id());
		CHECK(panel.paddingLeft == 4);
		CHECK(panel.paddingTop == 6);
		CHECK(panel.paddingRight == 8);
		CHECK(panel.paddingBottom == 10);
		CHECK(panel.tileCenter);
		CHECK(panel.tileEdges);
		CHECK_FALSE(panel.drawCenter);
	};
	check(*worldCopy);
	check(*uiCopy);
	CHECK(worldCopy->size == Vector2(120, 90));
	CHECK(worldCopy->offset == Vector2(5, 7));
	CHECK_FALSE(worldCopy->centered);
	REQUIRE(target.resources.Get<ImageTexture>(texture->Id()));
	CHECK(target.resources.Get<ImageTexture>(texture->Id())->IsLoaded());
}

TEST_CASE("UI: nine-patch editor creation and component dependencies support undo") {
	Fixture f;
	ScriptServer scripts(f.world);
	std::vector<editor::SceneDocumentSaveState> states;
	editor::UpdateSceneDocumentStates(states, {f.scene});
	const auto panel = editor::ExecuteCreateEntity(states, f.scene, f.canvas,
												   editor::EntityArchetype::UINinePatchRect);
	REQUIRE(panel.Has<UINinePatchRect>());
	REQUIRE(panel.Has<UIRect>());
	CHECK(editor::ArchetypeName(editor::EntityArchetype::UINinePatchRect) ==
		  std::string("UINinePatchRect"));
	REQUIRE(editor::UndoSceneDocument(states, f.scene));
	CHECK_FALSE(f.canvas.Find("UINinePatchRect").IsAlive());
	REQUIRE(editor::RedoSceneDocument(states, f.scene));
	CHECK(f.canvas.Find("UINinePatchRect").Has<UINinePatchRect>());

	const auto decoration = f.scene.Root().CreateChild("Decoration");
	REQUIRE(editor::ExecuteAddComponent(states, f.scene, decoration,
										f.world.FindComponent("NinePatchRect"), scripts));
	CHECK(decoration.Has<NinePatchRect>());
	CHECK(decoration.Has<Transform2D>());
	REQUIRE(editor::UndoSceneDocument(states, f.scene));
	CHECK_FALSE(decoration.Has<NinePatchRect>());
	CHECK_FALSE(decoration.Has<Transform2D>());
	REQUIRE(editor::RedoSceneDocument(states, f.scene));
	CHECK(decoration.Has<NinePatchRect>());
	CHECK(decoration.Has<Transform2D>());
}
