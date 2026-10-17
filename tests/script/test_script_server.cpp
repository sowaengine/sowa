#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>

#include <doctest/doctest.h>

#include "audio/audio_mixer.hpp"
#include "audio/audio_source.hpp"
#include "core/filesystem/file_system.hpp"
#include "core/input.hpp"
#include "core/log.hpp"
#include "core/project_settings.hpp"
#include "core/reflection/field_type.hpp"
#include "ecs/components/camera2d.hpp"
#include "ecs/components/render_order.hpp"
#include "ecs/components/transform2d.hpp"
#include "ecs/entity.hpp"
#include "ecs/world.hpp"
#include "math/color.hpp"
#include "math/vector2.hpp"
#include "math/vector3.hpp"
#include "physics/collider2d.hpp"
#include "physics/physics_world.hpp"
#include "physics/rigid_body2d.hpp"
#include "resource/resource.hpp"
#include "resource/resource_id.hpp"
#include "resource/resource_registry.hpp"
#include "scene/packed_scene.hpp"
#include "scene/scene_manager.hpp"
#include "script/lua_debugger.hpp"
#include "script/script_server.hpp"

using namespace sowa;

namespace {

struct Secret {
	f32 visible = 1.f;
	f32 hidden = 2.f;
};

struct Follower {
	Entity target;
};

struct Textured {
	ResourceID texture;
};

struct Label {
	std::string text;
};

struct Tint {
	Color modulate;
};

struct Offset3 {
	Vector3 offset;
};

enum class LuaAssetFilter {
	Nearest,
	Linear
};

struct LuaAsset : public Resource {
	std::string path;
	i32 pixelSize = 32;
	f32 volume = 1.f;
	bool flip = false;
	Vector2 size;
	Vector3 offset;
	Color tint;
	LuaAssetFilter filter = LuaAssetFilter::Nearest;
	ResourceID other;
	f32 hidden = 7.f;
	i32 reloads = 0;

	void Reload(const ResourceContext &) override {
		++reloads;
	}
};

struct LuaEngine {
	World world;
	Input input;
	FileSystem fs;
	ResourceRegistry resources;
	SceneManager scenes;
	ProjectSettings settings;
	ScriptServer scripts;

	LuaEngine() : scenes(world, resources, fs), scripts(world) {
		scripts.BindEngine(input, scenes, resources, settings);
	}
};

std::filesystem::path makeTempDir() {
	const auto dir = std::filesystem::temp_directory_path() / "sowa_lua_test";
	std::filesystem::remove_all(dir);
	std::filesystem::create_directories(dir);
	return dir;
}

void writeFile(const std::filesystem::path &path, std::string_view contents) {
	std::ofstream out(path, std::ios::binary);
	out.write(contents.data(), static_cast<std::streamsize>(contents.size()));
}

bool logHas(std::string_view needle) {
	for (const auto &line : Log::GetLines()) {
		if (line.text.find(std::string(needle)) != std::string::npos) {
			return true;
		}
	}
	return false;
}

} // namespace

namespace sowa {
SOWA_REFLECT_ENUM(LuaAssetFilter, {"Nearest", LuaAssetFilter::Nearest},
	{"Linear", LuaAssetFilter::Linear});
} // namespace sowa

namespace {

void describeLuaAsset(ResourceRegistry &registry) {
	registry.RegisterType<LuaAsset>("LuaAsset")
		.Field("path", &LuaAsset::path)
		.Field("pixelSize", &LuaAsset::pixelSize)
		.Field("volume", &LuaAsset::volume)
		.Field("flip", &LuaAsset::flip)
		.Field("size", &LuaAsset::size)
		.Field("offset", &LuaAsset::offset)
		.Field("tint", &LuaAsset::tint)
		.Field("filter", &LuaAsset::filter)
		.Field("other", &LuaAsset::other)
		.Field("hidden", &LuaAsset::hidden, FieldFlags::None);
}

} // namespace

TEST_CASE("DoString runs Lua and a Lua error does not unwind") {
	World world;
	ScriptServer scripts(world);
	CHECK(scripts.DoString("assert(world ~= nil)"));
	Log::Clear();
	CHECK_FALSE(scripts.DoString("error('boom')", "boom.lua"));
	REQUIRE_FALSE(Log::GetLines().empty());
	CHECK(Log::GetLines().back().text.find("boom") != std::string::npos);
}

TEST_CASE("Scripts can find and name entities") {
	World world;
	ScriptServer scripts(world);
	Entity e = world.CreateEntity();
	e.SetName("Player");
	scripts.SetEntity("e", e);

	CHECK(scripts.DoString("assert(e:is_alive())"));
	CHECK(scripts.DoString("assert(e.name == 'Player')"));
	CHECK(scripts.DoString("assert(world:find('Player') == e)"));
	CHECK(scripts.DoString("e.name = 'Hero'"));
	CHECK(e.GetName() == "Hero");
	CHECK(scripts.DoString("assert(world:find('missing') == nil)"));
}

TEST_CASE("create_entity from Lua is a live named entity") {
	World world;
	ScriptServer scripts(world);
	CHECK(scripts.DoString("e = world:create_entity('Spawned')"));
	Entity found = world.Find("Spawned");
	REQUIRE(found.IsAlive());
	CHECK(found.GetName() == "Spawned");
}

TEST_CASE("Transform2D fields round-trip through Lua") {
	World world;
	ScriptServer scripts(world);
	Entity e = world.CreateEntity();
	e.Add<Transform2D>();
	scripts.SetEntity("e", e);

	CHECK(scripts.DoString("local t = e:get('Transform2D'); t.rotation = 45"));
	CHECK(e.Get<Transform2D>()->rotation == doctest::Approx(45.f));

	CHECK(scripts.DoString("e:get('Transform2D').position = Vector2(10, 20)"));
	CHECK(e.Get<Transform2D>()->position.x == doctest::Approx(10.f));
	CHECK(e.Get<Transform2D>()->position.y == doctest::Approx(20.f));
}

TEST_CASE("Field proxies write back into component storage") {
	World world;
	ScriptServer scripts(world);
	Entity e = world.CreateEntity();
	e.Add<Transform2D>();
	scripts.SetEntity("e", e);

	CHECK(scripts.DoString("e:get('Transform2D').position.x = 20"));
	CHECK(e.Get<Transform2D>()->position.x == doctest::Approx(20.f));
	CHECK(e.Get<Transform2D>()->position.y == doctest::Approx(0.f));

	CHECK(scripts.DoString("local p = e:get('Transform2D').position; p.y = 50"));
	CHECK(e.Get<Transform2D>()->position.y == doctest::Approx(50.f));
}

TEST_CASE("Rebinding a proxy local does not write back") {
	World world;
	ScriptServer scripts(world);
	Entity e = world.CreateEntity();
	e.Add<Transform2D>();
	e.Get<Transform2D>()->position = Vector2(3.f, 4.f);
	scripts.SetEntity("e", e);

	CHECK(scripts.DoString("local p = e:get('Transform2D').position; p = Vector2(0, 0)"));
	CHECK(e.Get<Transform2D>()->position.x == doctest::Approx(3.f));
	CHECK(e.Get<Transform2D>()->position.y == doctest::Approx(4.f));
}

TEST_CASE("Proxy plus Vector2 assigns a whole new value") {
	World world;
	ScriptServer scripts(world);
	Entity e = world.CreateEntity();
	e.Add<Transform2D>();
	e.Get<Transform2D>()->position = Vector2(1.f, 2.f);
	scripts.SetEntity("e", e);

	CHECK(scripts.DoString(
		"local t = e:get('Transform2D'); t.position = t.position + Vector2(1, 0)"));
	CHECK(e.Get<Transform2D>()->position.x == doctest::Approx(2.f));
	CHECK(e.Get<Transform2D>()->position.y == doctest::Approx(2.f));
}

TEST_CASE("A component view re-resolves after an archetype move") {
	World world;
	ScriptServer scripts(world);
	Entity e = world.CreateEntity();
	e.Add<Transform2D>();
	scripts.SetEntity("e", e);

	CHECK(scripts.DoString("t = e:get('Transform2D')"));
	e.Add<Camera2D>();
	CHECK(scripts.DoString("t.position.x = 7"));
	CHECK(e.Get<Transform2D>()->position.x == doctest::Approx(7.f));
}

TEST_CASE("A field proxy re-resolves after an archetype move") {
	World world;
	ScriptServer scripts(world);
	Entity e = world.CreateEntity();
	e.Add<Transform2D>();
	scripts.SetEntity("e", e);

	CHECK(scripts.DoString("p = e:get('Transform2D').position"));
	e.Add<Camera2D>();
	CHECK(scripts.DoString("p.x = 9"));
	CHECK(e.Get<Transform2D>()->position.x == doctest::Approx(9.f));
}

TEST_CASE("A proxy write is a no-op once the component is gone") {
	World world;
	ScriptServer scripts(world);
	Entity e = world.CreateEntity();
	e.Add<Transform2D>();
	scripts.SetEntity("e", e);

	CHECK(scripts.DoString("p = e:get('Transform2D').position"));
	e.Remove<Transform2D>();
	CHECK(scripts.DoString("p.x = 20"));
	CHECK_FALSE(e.Has<Transform2D>());
}

TEST_CASE("Unexposed fields are unreachable from Lua") {
	World world;
	REQUIRE(world.RegisterComponent<Secret>("Secret")
			.Field("visible", &Secret::visible)
			.Field("hidden", &Secret::hidden, FieldFlags::None)
			.Result());
	ScriptServer scripts(world);
	Entity e = world.CreateEntity();
	e.Add(Secret{1.f, 2.f});
	scripts.SetEntity("e", e);

	CHECK(scripts.DoString("assert(e:get('Secret').hidden == nil)"));
	CHECK(scripts.DoString("e:get('Secret').hidden = 99"));
	CHECK(e.Get<Secret>()->hidden == doctest::Approx(2.f));
	CHECK(scripts.DoString("e:get('Secret').visible = 8"));
	CHECK(e.Get<Secret>()->visible == doctest::Approx(8.f));
}

TEST_CASE("Dead entity handles are safe no-ops") {
	World world;
	ScriptServer scripts(world);
	Entity e = world.CreateEntity();
	e.Add<Transform2D>();
	scripts.SetEntity("e", e);
	e.Destroy();

	CHECK(scripts.DoString("assert(not e:is_alive())"));
	CHECK(scripts.DoString("assert(e:get('Transform2D') == nil)"));
	CHECK(scripts.DoString("assert(e:has('Transform2D') == false)"));
	CHECK(scripts.DoString("e:add('Transform2D')"));
	CHECK(scripts.DoString("e:queue_destroy()"));
}

TEST_CASE("get/has/add/remove by registered type name") {
	World world;
	ScriptServer scripts(world);
	Entity e = world.CreateEntity();
	scripts.SetEntity("e", e);

	CHECK(scripts.DoString("assert(e:get('Transform2D') == nil)"));
	CHECK(scripts.DoString("assert(not e:has('Transform2D'))"));
	CHECK(scripts.DoString("e:add('Transform2D')"));
	REQUIRE(e.Has<Transform2D>());
	CHECK(scripts.DoString("assert(e:has('Transform2D'))"));
	CHECK(scripts.DoString("e:get('Transform2D').rotation = 12"));
	CHECK(e.Get<Transform2D>()->rotation == doctest::Approx(12.f));
	CHECK(scripts.DoString("e:remove('Transform2D')"));
	CHECK_FALSE(e.Has<Transform2D>());
}

TEST_CASE("Unknown component types do not panic") {
	World world;
	ScriptServer scripts(world);
	Entity e = world.CreateEntity();
	scripts.SetEntity("e", e);
	CHECK(scripts.DoString("assert(e:get('NoSuchType') == nil)"));
	CHECK(scripts.DoString("assert(not e:has('NoSuchType'))"));
	Log::Clear();
	CHECK(scripts.DoString("assert(e:add('NoSuchType') == nil)"));
	CHECK_FALSE(Log::GetLines().empty());
}

TEST_CASE("Data-described components use the same field walk") {
	World world;
	REQUIRE(world.RegisterComponent("Health", {{"hp", FieldType::F32}}));
	ScriptServer scripts(world);
	Entity e = world.CreateEntity();
	ComponentType health = world.FindComponent("Health");
	e.Add(health);
	scripts.SetEntity("e", e);

	CHECK(scripts.DoString("e:get('Health').hp = 25"));
	FieldRef hp = e.GetField(health, "hp");
	REQUIRE(hp.As<f32>() != nullptr);
	CHECK(*hp.As<f32>() == doctest::Approx(25.f));
}

TEST_CASE("Enum fields write by name") {
	World world;
	ScriptServer scripts(world);
	Entity e = world.CreateEntity();
	e.Add<RenderOrder>();
	scripts.SetEntity("e", e);

	CHECK(scripts.DoString("e:get('RenderOrder').zIndexMode = 'Absolute'"));
	CHECK(e.Get<RenderOrder>()->zIndexMode == ZIndexMode::Absolute);
	CHECK(scripts.DoString("assert(e:get('RenderOrder').zIndexMode == 'Absolute')"));
}

TEST_CASE("Entity-ref fields push and accept Entity userdata") {
	World world;
	REQUIRE(
		world.RegisterComponent<Follower>("Follower").Field("target", &Follower::target).Result());
	ScriptServer scripts(world);
	Entity a = world.CreateEntity();
	Entity b = world.CreateEntity();
	b.SetName("Target");
	a.Add<Follower>();
	scripts.SetEntity("a", a);
	scripts.SetEntity("b", b);

	CHECK(scripts.DoString("a:get('Follower').target = b"));
	CHECK(a.Get<Follower>()->target == b);
	CHECK(scripts.DoString("assert(a:get('Follower').target == b)"));
	CHECK(scripts.DoString("a:get('Follower').target = nil"));
	CHECK_FALSE(a.Get<Follower>()->target.IsAlive());
}

TEST_CASE("Resource-ref fields are ids, not owners") {
	World world;
	REQUIRE(world.RegisterComponent<Textured>("Textured")
			.Field("texture", &Textured::texture)
			.Result());
	ScriptServer scripts(world);
	Entity e = world.CreateEntity();
	e.Add<Textured>();
	scripts.SetEntity("e", e);

	CHECK(scripts.DoString("e:get('Textured').texture = 123"));
	CHECK(e.Get<Textured>()->texture.value == 123);
	CHECK(scripts.DoString("e:get('Textured').texture = nil"));
	CHECK(e.Get<Textured>()->texture.value == 0);
}

TEST_CASE("String fields round-trip") {
	World world;
	REQUIRE(world.RegisterComponent<Label>("Label").Field("text", &Label::text).Result());
	ScriptServer scripts(world);
	Entity e = world.CreateEntity();
	e.Add<Label>();
	scripts.SetEntity("e", e);

	CHECK(scripts.DoString("e:get('Label').text = 'hello'"));
	CHECK(e.Get<Label>()->text == "hello");
}

TEST_CASE("Color field proxies write back") {
	World world;
	REQUIRE(world.RegisterComponent<Tint>("Tint").Field("modulate", &Tint::modulate).Result());
	ScriptServer scripts(world);
	Entity e = world.CreateEntity();
	e.Add<Tint>();
	scripts.SetEntity("e", e);

	CHECK(scripts.DoString("e:get('Tint').modulate.r = 0.5"));
	CHECK(e.Get<Tint>()->modulate.r == doctest::Approx(0.5f));
	CHECK(scripts.DoString("e:get('Tint').modulate = Color(0, 1, 0, 1)"));
	CHECK(e.Get<Tint>()->modulate.g == doctest::Approx(1.f));
}

TEST_CASE("Color rgb and hsv factories") {
	World world;
	REQUIRE(world.RegisterComponent<Tint>("Tint").Field("modulate", &Tint::modulate).Result());
	ScriptServer scripts(world);
	Entity e = world.CreateEntity();
	e.Add<Tint>();
	scripts.SetEntity("e", e);

	CHECK(scripts.DoString(R"(
		local white = Color()
		assert(white.r == 1 and white.g == 1 and white.b == 1 and white.a == 1)
		local gray = Color(0.5)
		assert(gray.r == 0.5 and gray.a == 1)
		local rgb = Color(0.1, 0.2, 0.3)
		assert(rgb.a == 1)
		local rgba = Color(0.1, 0.2, 0.3, 0.4)
		assert(math.abs(rgba.a - 0.4) < 0.0001)
		assert(Color.rgb(255, 0, 0) == Color(1, 0, 0))
		local orange = Color.rgb(255, 128, 0)
		assert(math.abs(orange.r - 1) < 0.0001)
		assert(math.abs(orange.g - 128 / 255) < 0.0001)
		assert(math.abs(orange.b) < 0.0001)
		assert(math.abs(orange.a - 1) < 0.0001)
		local faded = Color.rgb(0, 0, 255, 128)
		assert(math.abs(faded.b - 1) < 0.0001)
		assert(math.abs(faded.a - 128 / 255) < 0.0001)
		assert(Color.hsv(0, 1, 1) == Color(1, 0, 0))
		local green = Color.hsv(120, 1, 1)
		assert(math.abs(green.g - 1) < 0.0001)
		assert(math.abs(green.r) < 0.0001)
		assert(math.abs(green.b) < 0.0001)
		assert(math.abs(green.a - 1) < 0.0001)
		e:get('Tint').modulate = Color.rgb(0, 255, 0)
	)"));
	CHECK(e.Get<Tint>()->modulate.g == doctest::Approx(1.f));
	CHECK(e.Get<Tint>()->modulate.r == doctest::Approx(0.f));
	CHECK(e.Get<Tint>()->modulate.b == doctest::Approx(0.f));
}

TEST_CASE("Vector3 field proxies write back") {
	World world;
	REQUIRE(world.RegisterComponent<Offset3>("Offset3").Field("offset", &Offset3::offset).Result());
	ScriptServer scripts(world);
	Entity e = world.CreateEntity();
	e.Add<Offset3>();
	scripts.SetEntity("e", e);

	CHECK(scripts.DoString("e:get('Offset3').offset.z = 3"));
	CHECK(e.Get<Offset3>()->offset.z == doctest::Approx(3.f));
}

TEST_CASE("queue_destroy is flushed by Progress") {
	World world;
	ScriptServer scripts(world);
	Entity e = world.CreateEntity();
	scripts.SetEntity("e", e);
	CHECK(scripts.DoString("e:queue_destroy()"));
	CHECK(e.IsAlive());
	world.Progress(0.f);
	CHECK_FALSE(e.IsAlive());
}

TEST_CASE("Parent property round-trips") {
	World world;
	ScriptServer scripts(world);
	Entity parent = world.CreateEntity();
	parent.SetName("Root");
	Entity child = world.CreateEntity();
	child.SetName("Kid");
	scripts.SetEntity("parent", parent);
	scripts.SetEntity("child", child);

	CHECK(scripts.DoString("child.parent = parent"));
	CHECK(child.GetParent() == parent);
	CHECK(scripts.DoString("assert(child.parent == parent)"));
	CHECK(scripts.DoString("child.parent = nil"));
	CHECK_FALSE(child.HasParent());
}

TEST_CASE("DoFile runs a script loaded through FileSystem") {
	const auto root = makeTempDir();
	writeFile(root / "move.lua", "e:get('Transform2D').position.x = 11\n");

	FileSystem fs;
	fs.MountFolder("res", root);

	World world;
	ScriptServer scripts(world);
	Entity e = world.CreateEntity();
	e.Add<Transform2D>();
	scripts.SetEntity("e", e);

	CHECK(scripts.DoFile(fs, "res://move.lua"));
	CHECK(e.Get<Transform2D>()->position.x == doctest::Approx(11.f));
}

TEST_CASE("DoFile of a missing path is a logged failure") {
	FileSystem fs;
	fs.MountFolder("res", makeTempDir());
	World world;
	ScriptServer scripts(world);
	Log::Clear();
	CHECK_FALSE(scripts.DoFile(fs, "res://missing.lua"));
	CHECK_FALSE(Log::GetLines().empty());
}

TEST_CASE("Lua files own the script components they declare") {
	const auto root = makeTempDir();
	writeFile(root / "behavior.lua", R"(
world:component("Mover", { speed = 3 })
world:component("Health", { amount = 10 })
)");
	writeFile(root / "main.lua", R"(
require("res://behavior.lua")
world:component("Game", {})
)");

	FileSystem fs;
	fs.MountFolder("res", root);
	World world;
	ScriptServer scripts(world);
	REQUIRE(scripts.LoadModules(fs, {"res://main.lua"}));

	const std::vector<ComponentType> behavior = scripts.ScriptComponentTypes("res://behavior.lua");
	REQUIRE(behavior.size() == 2);
	CHECK(behavior[0].Name() == "Mover");
	CHECK(behavior[1].Name() == "Health");
	CHECK(scripts.IsProjectScriptSource("res://main.lua"));
	CHECK(scripts.IsProjectScriptSource("res://behavior.lua"));

	Entity entity = world.CreateEntity();
	REQUIRE(scripts.AttachScript(entity, "res://behavior.lua"));
	CHECK(entity.Has(world.FindComponent("Mover")));
	CHECK(entity.Has(world.FindComponent("Health")));
	f32 speed = 0.f;
	REQUIRE(entity.GetField(world.FindComponent("Mover"), "speed").Get(speed));
	CHECK(speed == doctest::Approx(3.f));
	CHECK(scripts.AttachedScriptPaths(entity) == std::vector<std::string>{"res://behavior.lua"});

	CHECK(scripts.RemoveScript(entity, "res://behavior.lua"));
	CHECK_FALSE(entity.Has(world.FindComponent("Mover")));
	CHECK_FALSE(entity.Has(world.FindComponent("Health")));
}

TEST_CASE("require loads a VFS module and returns its value") {
	const auto root = makeTempDir();
	writeFile(root / "mod.lua", "return 42\n");

	FileSystem fs;
	fs.MountFolder("res", root);
	World world;
	ScriptServer scripts(world);
	REQUIRE(scripts.LoadModules(fs, {}));
	CHECK(scripts.DoString("assert(require('res://mod.lua') == 42)"));
	CHECK(scripts.DoString("assert(require('res:///mod.lua') == 42)"));
	CHECK(scripts.DoString("assert(select(1, require('res://mod.lua')) == 42)"));
}

TEST_CASE("require caches the first return value") {
	const auto root = makeTempDir();
	writeFile(root / "counter.lua", "n = (n or 0) + 1; return n\n");

	FileSystem fs;
	fs.MountFolder("res", root);
	World world;
	ScriptServer scripts(world);
	REQUIRE(scripts.LoadModules(fs, {}));
	CHECK(scripts.DoString("assert(require('res://counter.lua') == 1)"));
	CHECK(scripts.DoString("assert(require('res://counter.lua') == 1)"));
	CHECK(scripts.DoString("assert(require('res:///counter.lua') == 1)"));
	CHECK(scripts.DoString("assert(n == 1)"));
}

TEST_CASE("require of a module with no return yields true") {
	const auto root = makeTempDir();
	writeFile(root / "side.lua", "side_ran = true\n");

	FileSystem fs;
	fs.MountFolder("res", root);
	World world;
	ScriptServer scripts(world);
	REQUIRE(scripts.LoadModules(fs, {}));
	CHECK(scripts.DoString("assert(require('res://side.lua') == true)"));
	CHECK(scripts.DoString("assert(side_ran == true)"));
}

TEST_CASE("require passes the canonical path as ...") {
	const auto root = makeTempDir();
	writeFile(root / "name.lua", "return ...\n");

	FileSystem fs;
	fs.MountFolder("res", root);
	World world;
	ScriptServer scripts(world);
	REQUIRE(scripts.LoadModules(fs, {}));
	CHECK(scripts.DoString("assert(require('res:///name.lua') == 'res://name.lua')"));
}

TEST_CASE("require of a missing path is a Lua error") {
	FileSystem fs;
	fs.MountFolder("res", makeTempDir());
	World world;
	ScriptServer scripts(world);
	REQUIRE(scripts.LoadModules(fs, {}));
	Log::Clear();
	CHECK_FALSE(scripts.DoString("require('res://missing.lua')"));
	CHECK(logHas("failed to load"));
	CHECK(scripts.DoString("local ok = pcall(require, 'res://missing.lua'); assert(ok == false)"));
}

TEST_CASE("require without a scheme does not load an OS file") {
	const auto root = makeTempDir();
	writeFile(root / "evil.lua", "return 1\n");

	FileSystem fs;
	fs.MountFolder("res", root);
	World world;
	ScriptServer scripts(world);
	REQUIRE(scripts.LoadModules(fs, {}));
	Log::Clear();
	CHECK_FALSE(scripts.DoString("require('evil')"));
	CHECK(logHas("invalid path"));
	CHECK_FALSE(scripts.DoString("require('evil.lua')"));
}

TEST_CASE("require rejects a path escape") {
	FileSystem fs;
	fs.MountFolder("res", makeTempDir());
	World world;
	ScriptServer scripts(world);
	REQUIRE(scripts.LoadModules(fs, {}));
	Log::Clear();
	CHECK_FALSE(scripts.DoString("require('res://../secret.lua')"));
	CHECK(logHas("invalid path"));
}

TEST_CASE("package library stays closed") {
	World world;
	ScriptServer scripts(world);
	CHECK(scripts.DoString("assert(package.path == nil)"));
	CHECK(scripts.DoString("assert(package.cpath == nil)"));
	CHECK(scripts.DoString("assert(package.loadlib == nil)"));
	CHECK(scripts.DoString("assert(package.searchers == nil)"));
	CHECK(scripts.DoString("assert(type(package.loaded) == 'table')"));
	CHECK(scripts.DoString("assert(loadfile == nil)"));
	CHECK(scripts.DoString("assert(dofile == nil)"));
}

TEST_CASE("require without a filesystem is a Lua error") {
	World world;
	ScriptServer scripts(world);
	Log::Clear();
	CHECK_FALSE(scripts.DoString("require('res://x.lua')"));
	CHECK(logHas("no filesystem"));
}

TEST_CASE("require cycle is a Lua error") {
	const auto root = makeTempDir();
	writeFile(root / "a.lua", "require('res://b.lua')\n");
	writeFile(root / "b.lua", "require('res://a.lua')\n");

	FileSystem fs;
	fs.MountFolder("res", root);
	World world;
	ScriptServer scripts(world);
	REQUIRE(scripts.LoadModules(fs, {}));
	Log::Clear();
	CHECK_FALSE(scripts.DoString("require('res://a.lua')"));
	CHECK(logHas("require cycle"));
	CHECK(logHas("res://a.lua -> res://b.lua -> res://a.lua"));
}

TEST_CASE("package.loaded early assign breaks a require cycle") {
	const auto root = makeTempDir();
	writeFile(root / "a.lua", R"(
local M = { n = 1 }
package.loaded[...] = M
local B = require('res://b.lua')
M.from_b = B.n
return M
)");
	writeFile(root / "b.lua", R"(
local A = require('res://a.lua')
return { n = A.n }
)");

	FileSystem fs;
	fs.MountFolder("res", root);
	World world;
	ScriptServer scripts(world);
	REQUIRE(scripts.LoadModules(fs, {}));
	CHECK(scripts.DoString(
		"local A = require('res://a.lua'); assert(A.n == 1); assert(A.from_b == 1)"));
}

TEST_CASE("ReloadModules re-runs required files") {
	const auto root = makeTempDir();
	writeFile(root / "val.lua", "return 1\n");
	writeFile(root / "main.lua", "n = require('res://val.lua')\n");

	FileSystem fs;
	fs.MountFolder("res", root);
	World world;
	ScriptServer scripts(world);
	REQUIRE(scripts.LoadModules(fs, {"res://main.lua"}));
	CHECK(scripts.DoString("assert(n == 1)"));

	writeFile(root / "val.lua", "return 2\n");
	REQUIRE(scripts.ReloadModules(fs));
	CHECK(scripts.DoString("assert(n == 2)"));
}

TEST_CASE("ReloadModules can replace and clear the project module list") {
	const auto root = makeTempDir();
	writeFile(root / "a.lua", "which = 'a'\n");
	writeFile(root / "b.lua", "which = 'b'\n");

	FileSystem fs;
	fs.MountFolder("res", root);
	World world;
	ScriptServer scripts(world);
	REQUIRE(scripts.LoadModules(fs, {"res://a.lua"}));
	CHECK(scripts.IsProjectScriptSource("res://a.lua"));

	REQUIRE(scripts.ReloadModules(fs, {"res://b.lua"}));
	CHECK(scripts.DoString("assert(which == 'b')"));
	CHECK_FALSE(scripts.IsProjectScriptSource("res://a.lua"));
	CHECK(scripts.IsProjectScriptSource("res://b.lua"));

	REQUIRE(scripts.ReloadModules(fs, {}));
	CHECK_FALSE(scripts.IsProjectScriptSource("res://b.lua"));
}

TEST_CASE("children() returns insertion-order handles") {
	World world;
	ScriptServer scripts(world);
	Entity root = world.CreateEntity();
	root.SetName("Root");
	Entity a = root.CreateChild("A");
	Entity b = root.CreateChild("B");
	scripts.SetEntity("root", root);

	CHECK(scripts.DoString("kids = root:children(); assert(#kids == 2)"));
	CHECK(scripts.DoString("assert(kids[1] == world:find('Root/A'))"));
	CHECK(scripts.DoString("assert(kids[2] == world:find('Root/B'))"));
	(void)a;
	(void)b;
}

TEST_CASE("visible property toggles Hidden") {
	World world;
	ScriptServer scripts(world);
	Entity e = world.CreateEntity();
	scripts.SetEntity("e", e);

	CHECK(scripts.DoString("assert(e.visible == true)"));
	CHECK(scripts.DoString("e.visible = false"));
	CHECK_FALSE(e.IsVisible());
	CHECK(scripts.DoString("assert(e.visible == false)"));
	CHECK(scripts.DoString("e.visible = true"));
	CHECK(e.IsVisible());
}

TEST_CASE("Call invokes a global function and missing is not an error") {
	World world;
	ScriptServer scripts(world);
	CHECK(scripts.DoString("n = 0; function _update(dt) n = dt end"));
	CHECK(scripts.Call("_update", 0.25f));
	CHECK(scripts.DoString("assert(n == 0.25)"));
	CHECK(scripts.Call("does_not_exist"));
}

TEST_CASE("Call logs a Lua error and does not unwind") {
	World world;
	ScriptServer scripts(world);
	CHECK(scripts.DoString("function boom() error('nope') end"));
	Log::Clear();
	CHECK_FALSE(scripts.Call("boom"));
	REQUIRE_FALSE(Log::GetLines().empty());
	CHECK(Log::GetLines().back().text.find("nope") != std::string::npos);
}

TEST_CASE("math binds engine Math and not Lua's math library") {
	World world;
	ScriptServer scripts(world);
	CHECK(scripts.DoString(R"(
		assert(math.huge == nil)
		local r = math.random()
		assert(r >= 0 and r < 1)
		local a = math.random(2, 3)
		assert(a >= 2 and a < 3)
		assert(math.abs(-4) == 4)
		assert(math.abs(math.sin(0)) < 0.0001)
		assert(math.abs(math.cos(0) - 1) < 0.0001)
		assert(math.abs(math.sqrt(9) - 3) < 0.0001)
		assert(math.abs(math.clamp(15, 0, 10) - 10) < 0.0001)
		assert(math.abs(math.lerp(0, 10, 0.5) - 5) < 0.0001)
		assert(math.abs(math.lerp(0, 10, 0.5, Easing.QuadIn) - 2.5) < 0.0001)
		assert(math.abs(math.map(5, 0, 10, 0, 100) - 50) < 0.0001)
		assert(math.abs(math.tween_map(0.5, 0, 100) - 50) < 0.0001)
		assert(math.abs(math.ease(0.5, Easing.QuadIn) - 0.25) < 0.0001)
		assert(math.abs(math.ease(0.5, Easing.Linear) - 0.5) < 0.0001)
		assert(Easing.Linear == 0)
		assert(Easing.Count == nil)
		assert(math.abs(math.atan2(0, 1)) < 0.0001)
		assert(math.abs(math.pi - 3.141592) < 0.001)
		local p = math.lerp(Vector2(0, 0), Vector2(10, 0), 0.5)
		assert(math.abs(p.x - 5) < 0.0001)
	)"));
}

TEST_CASE("math.lerp accepts a FieldProxy Vector2") {
	World world;
	ScriptServer scripts(world);
	Entity e = world.CreateEntity();
	e.Add<Transform2D>();
	e.Get<Transform2D>()->position = Vector2(0.f, 0.f);
	scripts.SetEntity("e", e);
	CHECK(scripts.DoString(R"(
		local p = math.lerp(e:get('Transform2D').position, Vector2(10, 0), 0.5)
		assert(math.abs(p.x - 5) < 0.0001)
	)"));
}

TEST_CASE("Rect is a Lua value type with edges contains and map_point") {
	World world;
	ScriptServer scripts(world);
	CHECK(scripts.DoString(R"(
		local r = Rect(1, 2, 10, 20)
		assert(r.x == 1 and r.y == 2 and r.w == 10 and r.h == 20)
		assert(r:left() == 1)
		assert(r:right() == 11)
		assert(r:bottom() == 2)
		assert(r:top() == 22)
		assert(r:contains(Vector2(5, 5)))
		assert(not r:contains(Vector2(20, 5)))
		assert(r:intersects(Rect(8, 8, 10, 10)))
		assert(not r:intersects(Rect(100, 100, 5, 5)))
		local p = Rect(0, 0, 10, 10):map_point(Vector2(5, 5), Rect(0, 0, 100, 100))
		assert(math.abs(p.x - 50) < 0.0001)
		assert(math.abs(p.y - 50) < 0.0001)
		local empty = Rect()
		assert(empty.w == 0 and empty.h == 0)
		local corner = Rect(3, 4)
		assert(corner.x == 3 and corner.y == 4 and corner.w == 0)
	)"));
}

TEST_CASE("world.delta_time is the last Progress argument") {
	World world;
	ScriptServer scripts(world);
	CHECK(scripts.DoString("assert(math.abs(world.delta_time) < 0.0001)"));
	world.Progress(0.125f);
	CHECK(scripts.DoString("assert(math.abs(world.delta_time - 0.125) < 0.0001)"));
	world.Progress(0.5f);
	CHECK(scripts.DoString("assert(math.abs(world.delta_time - 0.5) < 0.0001)"));
}

TEST_CASE("log.info warn and error write the matching severity") {
	World world;
	Input input;
	FileSystem fs;
	ResourceRegistry resources;
	SceneManager scenes(world, resources, fs);
	ProjectSettings settings;
	ScriptServer scripts(world);
	scripts.BindEngine(input, scenes, resources, settings);

	Log::Clear();
	REQUIRE(scripts.DoString("log.info('hello from lua')"));
	REQUIRE(Log::GetLines().size() == 1);
	CHECK(Log::GetLines().back().severity == Log::Severity::Info);
	CHECK(Log::GetLines().back().text.find("hello from lua") != std::string::npos);

	Log::Clear();
	REQUIRE(scripts.DoString("log.warn('careful')"));
	REQUIRE(Log::GetLines().size() == 1);
	CHECK(Log::GetLines().back().severity == Log::Severity::Warn);
	CHECK(Log::GetLines().back().text.find("careful") != std::string::npos);

	Log::Clear();
	REQUIRE(scripts.DoString("log.error('broken')"));
	REQUIRE(Log::GetLines().size() == 1);
	CHECK(Log::GetLines().back().severity == Log::Severity::Error);
	CHECK(Log::GetLines().back().text.find("broken") != std::string::npos);
}

TEST_CASE("log.info formats extra arguments with {}") {
	World world;
	Input input;
	FileSystem fs;
	ResourceRegistry resources;
	SceneManager scenes(world, resources, fs);
	ProjectSettings settings;
	ScriptServer scripts(world);
	scripts.BindEngine(input, scenes, resources, settings);

	Log::Clear();
	REQUIRE(scripts.DoString("log.info('hello {}', 'lua')"));
	REQUIRE(Log::GetLines().size() == 1);
	CHECK(Log::GetLines().back().severity == Log::Severity::Info);
	CHECK(Log::GetLines().back().text.find("hello lua") != std::string::npos);
	CHECK(Log::GetLines().back().text.find("hello {}") == std::string::npos);

	Log::Clear();
	REQUIRE(scripts.DoString("log.info('count={}', 3)"));
	REQUIRE_FALSE(Log::GetLines().empty());
	CHECK(Log::GetLines().back().text.find("count=3") != std::string::npos);

	Log::Clear();
	REQUIRE(scripts.DoString("log.warn('flag={}', true)"));
	REQUIRE_FALSE(Log::GetLines().empty());
	CHECK(Log::GetLines().back().severity == Log::Severity::Warn);
	CHECK(Log::GetLines().back().text.find("flag=true") != std::string::npos);

	Log::Clear();
	REQUIRE(scripts.DoString("log.info('{} {}', 1)"));
	REQUIRE_FALSE(Log::GetLines().empty());
	CHECK(Log::GetLines().back().severity == Log::Severity::Error);
	CHECK(Log::GetLines().back().text.find("log format:") != std::string::npos);
}

TEST_CASE("settings exposes main_scene and viewport size") {
	World world;
	Input input;
	FileSystem fs;
	ResourceRegistry resources;
	SceneManager scenes(world, resources, fs);
	ProjectSettings settings;
	settings.application.mainScene = "res://scenes/demo.scene";
	settings.rendering.window.width = 800;
	settings.rendering.window.height = 600;
	settings.rendering.viewport.width = 1920;
	settings.rendering.viewport.height = 1080;
	ScriptServer scripts(world);
	scripts.BindEngine(input, scenes, resources, settings);

	CHECK(scripts.DoString(R"(
		assert(settings.main_scene == 'res://scenes/demo.scene')
		assert(settings.viewport_width == 1920)
		assert(settings.viewport_height == 1080)
	)"));

	settings.application.mainScene = "res://scenes/next.scene";
	settings.rendering.viewport.width = 640;
	settings.rendering.viewport.height = 360;
	scripts.UpdateSettings(settings);
	CHECK(scripts.DoString(R"(
		assert(settings.main_scene == 'res://scenes/next.scene')
		assert(settings.viewport_width == 640)
		assert(settings.viewport_height == 360)
	)"));
}

TEST_CASE("global_store exposes the live project string store") {
	World world;
	Input input;
	FileSystem fs;
	ResourceRegistry resources;
	SceneManager scenes(world, resources, fs);
	ProjectSettings settings;
	StringStore store;
	store.Set("lives", "3");
	ScriptServer scripts(world);
	scripts.BindEngine(input, scenes, resources, settings, nullptr, nullptr, &store);

	CHECK(scripts.DoString(R"(
		assert(global_store:has('lives'))
		assert(global_store:get('lives') == '3')
		global_store:set('lives', '2')
		global_store:set('score', '100')
		global_store:remove('score')
	)"));
	CHECK(store.Get("lives") == "2");
	CHECK_FALSE(store.Has("score"));
}

TEST_CASE("input polls keys, mouse_position, and Key codes") {
	World world;
	Input input;
	FileSystem fs;
	ResourceRegistry resources;
	SceneManager scenes(world, resources, fs);
	ProjectSettings settings;
	ScriptServer scripts(world);
	scripts.BindEngine(input, scenes, resources, settings);

	CHECK(scripts.DoString("assert(Key.Space == 32)"));
	CHECK(scripts.DoString("assert(Key.A == 65)"));
	CHECK(scripts.DoString("assert(Key.F13 == 302)"));
	CHECK(scripts.DoString("assert(Key.KeypadEnter == 335)"));
	CHECK(scripts.DoString("assert(Key.LeftSuper == 343)"));
	CHECK(scripts.DoString("assert(Key.Mouse4 == 10004)"));
	CHECK(scripts.DoString("assert(Key.MouseLeft == Key.Mouse1)"));

	input.HandleKey(Key::Space, KeyAction::Press);
	CHECK(scripts.DoString("assert(input:is_key_just_pressed(Key.Space))"));
	CHECK(scripts.DoString("assert(input:is_key_down(Key.Space))"));
	CHECK(scripts.DoString("assert(not input:is_key_down(Key.A))"));
	input.Update();
	CHECK(scripts.DoString("assert(not input:is_key_just_pressed(Key.Space))"));
	CHECK(scripts.DoString("assert(input:is_key_down(Key.Space))"));
	input.HandleKey(Key::Space, KeyAction::Release);
	CHECK(scripts.DoString("assert(input:is_key_just_released(Key.Space))"));
	CHECK(scripts.DoString("assert(not input:is_key_down(Key.Space))"));

	input.HandleCursorPos(12.f, 34.f);
	CHECK(scripts.DoString(R"(
		local p = input:mouse_position()
		assert(math.abs(p.x - 12) < 0.0001)
		assert(math.abs(p.y - 34) < 0.0001)
	)"));
}

TEST_CASE("resources:find_packed_scene returns the matching id") {
	World world;
	Input input;
	FileSystem fs;
	ResourceRegistry resources;
	resources.RegisterType<PackedScene>("PackedScene").Field("path", &PackedScene::path);
	SceneManager scenes(world, resources, fs);
	ProjectSettings settings;
	ScriptServer scripts(world);
	scripts.BindEngine(input, scenes, resources, settings);

	Ref<PackedScene> packed = resources.Create<PackedScene>(ResourceID{42});
	REQUIRE(packed != nullptr);
	packed->path = "res://scenes/object.scene";
	CHECK(
		scripts.DoString("assert(resources:find_packed_scene('res://scenes/object.scene') == 42)"));
	CHECK(scripts.DoString("assert(resources:find_packed_scene('res://missing.scene') == nil)"));
	CHECK(scripts.DoString("assert(resources:get(42).path == 'res://scenes/object.scene')"));
}

TEST_CASE("resources:get is nil for a missing id") {
	LuaEngine engine;
	describeLuaAsset(engine.resources);

	CHECK(engine.scripts.DoString("assert(resources:get(nil) == nil)"));
	CHECK(engine.scripts.DoString("assert(resources:get(0) == nil)"));
	CHECK(engine.scripts.DoString("assert(resources:get(1) == nil)"));
}

TEST_CASE("resources:get walks described fields like a ComponentView") {
	LuaEngine engine;
	describeLuaAsset(engine.resources);
	Ref<LuaAsset> asset = engine.resources.Create<LuaAsset>(ResourceID{1});
	REQUIRE(asset != nullptr);
	asset->path = "res://a.png";
	asset->pixelSize = 16;
	asset->volume = 0.5f;
	asset->flip = true;
	asset->size = Vector2(4.f, 8.f);
	asset->offset = Vector3(1.f, 2.f, 3.f);
	asset->tint = Color(0.25f, 0.5f, 0.75f, 1.f);
	asset->filter = LuaAssetFilter::Linear;
	asset->other = ResourceID{99};

	CHECK(engine.scripts.DoString(R"(
		local r = resources:get(1)
		assert(r ~= nil)
		assert(resources:get(1) == r)
		assert(r.path == 'res://a.png')
		assert(r.pixelSize == 16)
		assert(math.abs(r.volume - 0.5) < 0.0001)
		assert(r.flip == true)
		assert(math.abs(r.size.x - 4) < 0.0001)
		assert(math.abs(r.size.y - 8) < 0.0001)
		assert(math.abs(r.offset.z - 3) < 0.0001)
		assert(math.abs(r.tint.g - 0.5) < 0.0001)
		assert(r.filter == 'Linear')
		assert(r.other == 99)
	)"));
	CHECK(asset->reloads == 0);

	CHECK(engine.scripts.DoString(R"(
		local r = resources:get(1)
		r.path = 'res://b.png'
		r.pixelSize = 24
		r.volume = 0.25
		r.flip = false
		r.size = Vector2(10, 20)
		r.offset.z = 9
		r.tint.r = 0.1
		r.filter = 'Nearest'
		r.other = 7
	)"));
	CHECK(asset->path == "res://b.png");
	CHECK(asset->pixelSize == 24);
	CHECK(asset->volume == doctest::Approx(0.25f));
	CHECK_FALSE(asset->flip);
	CHECK(asset->size.x == doctest::Approx(10.f));
	CHECK(asset->size.y == doctest::Approx(20.f));
	CHECK(asset->offset.z == doctest::Approx(9.f));
	CHECK(asset->tint.r == doctest::Approx(0.1f));
	CHECK(asset->filter == LuaAssetFilter::Nearest);
	CHECK(asset->other.value == 7);
	CHECK(asset->reloads == 9);
}

TEST_CASE("A resource field proxy writes back and reloads") {
	LuaEngine engine;
	describeLuaAsset(engine.resources);
	Ref<LuaAsset> asset = engine.resources.Create<LuaAsset>(ResourceID{1});
	REQUIRE(asset != nullptr);
	asset->size = Vector2(1.f, 2.f);

	CHECK(engine.scripts.DoString("resources:get(1).size.x = 8"));
	CHECK(asset->size.x == doctest::Approx(8.f));
	CHECK(asset->size.y == doctest::Approx(2.f));
	CHECK(asset->reloads == 1);

	CHECK(engine.scripts.DoString("resources:get(1).size = resources:get(1).size + Vector2(1, 0)"));
	CHECK(asset->size.x == doctest::Approx(9.f));
	CHECK(asset->reloads == 2);
}

TEST_CASE("Unexposed resource fields are unreachable from Lua") {
	LuaEngine engine;
	describeLuaAsset(engine.resources);
	Ref<LuaAsset> asset = engine.resources.Create<LuaAsset>(ResourceID{1});
	REQUIRE(asset != nullptr);
	asset->hidden = 7.f;

	CHECK(engine.scripts.DoString("assert(resources:get(1).hidden == nil)"));
	CHECK(engine.scripts.DoString("resources:get(1).hidden = 99"));
	CHECK(asset->hidden == doctest::Approx(7.f));
	CHECK(asset->reloads == 0);
}

TEST_CASE("A stale resource view is a safe no-op") {
	LuaEngine engine;
	describeLuaAsset(engine.resources);
	Ref<LuaAsset> asset = engine.resources.Create<LuaAsset>(ResourceID{1});
	REQUIRE(asset != nullptr);
	asset->path = "res://keep.png";

	CHECK(engine.scripts.DoString("r = resources:get(1)"));
	REQUIRE(engine.resources.Remove(ResourceID{1}));
	CHECK(engine.scripts.DoString("assert(resources:get(1) == nil)"));
	CHECK(engine.scripts.DoString("assert(r.path == nil)"));
	CHECK(engine.scripts.DoString("r.path = 'res://gone.png'"));
	CHECK(engine.scripts.DoString("r.size = Vector2(1, 1)"));
	CHECK(asset->path == "res://keep.png");
	CHECK(asset->reloads == 0);
}

TEST_CASE("A component ResourceRef is an id that resources:get can walk") {
	LuaEngine engine;
	describeLuaAsset(engine.resources);
	REQUIRE(engine.world.RegisterComponent<Textured>("Textured")
			.Field("texture", &Textured::texture)
			.Result());
	Ref<LuaAsset> asset = engine.resources.Create<LuaAsset>(ResourceID{5});
	REQUIRE(asset != nullptr);
	asset->path = "res://icon.png";
	Entity e = engine.world.CreateEntity();
	e.Add<Textured>();
	e.Get<Textured>()->texture = ResourceID{5};
	engine.scripts.SetEntity("e", e);

	CHECK(engine.scripts.DoString("assert(e:get('Textured').texture == 5)"));
	CHECK(engine.scripts.DoString(
		"assert(resources:get(e:get('Textured').texture).path == 'res://icon.png')"));
}

TEST_CASE("timers:create one-shot fires on_timeout once") {
	World world;
	ScriptServer scripts(world);
	REQUIRE(scripts.DoString(R"(
		n = 0
		t = timers:create(0.5, true)
		t:on_timeout(function() n = n + 1 end)
		assert(t:is_running())
		assert(not t:is_paused())
		assert(t.repeating == false)
	)"));
	scripts.UpdateTimers(0.4f);
	CHECK(scripts.DoString("assert(n == 0)"));
	CHECK(scripts.DoString("assert(math.abs(t.remaining - 0.1) < 0.0001)"));
	scripts.UpdateTimers(0.2f);
	CHECK(scripts.DoString("assert(n == 1)"));
	CHECK(scripts.DoString("assert(not t:is_running())"));
	scripts.UpdateTimers(1.f);
	CHECK(scripts.DoString("assert(n == 1)"));
}

TEST_CASE("timers:create repeating catches up on a large delta") {
	World world;
	ScriptServer scripts(world);
	REQUIRE(scripts.DoString(R"(
		n = 0
		t = timers:create(0.1, true, true)
		t:on_timeout(function() n = n + 1 end)
		assert(t.repeating == true)
	)"));
	scripts.UpdateTimers(0.5f);
	CHECK(scripts.DoString("assert(n == 5)"));
	CHECK(scripts.DoString("assert(t:is_running())"));
}

TEST_CASE("timer pause holds elapsed and stop resets it") {
	World world;
	ScriptServer scripts(world);
	REQUIRE(scripts.DoString(R"(
		n = 0
		t = timers:create(1, true)
		t:on_timeout(function() n = n + 1 end)
	)"));
	scripts.UpdateTimers(0.4f);
	CHECK(scripts.DoString("assert(math.abs(t.elapsed - 0.4) < 0.0001)"));
	CHECK(scripts.DoString("t:pause(); assert(t:is_paused())"));
	scripts.UpdateTimers(0.5f);
	CHECK(scripts.DoString("assert(n == 0)"));
	CHECK(scripts.DoString("assert(math.abs(t.elapsed - 0.4) < 0.0001)"));
	CHECK(scripts.DoString("t:start(); assert(t:is_running())"));
	scripts.UpdateTimers(0.6f);
	CHECK(scripts.DoString("assert(n == 1)"));
	CHECK(scripts.DoString("t:stop(); assert(not t:is_running()); assert(t.elapsed == 0)"));
}

TEST_CASE("timer:on_timeout(nil) clears the callback") {
	World world;
	ScriptServer scripts(world);
	REQUIRE(scripts.DoString(R"(
		n = 0
		t = timers:create(0.1, true)
		t:on_timeout(function() n = n + 1 end)
	)"));
	CHECK(scripts.DoString("t:on_timeout(nil)"));
	scripts.UpdateTimers(0.2f);
	CHECK(scripts.DoString("assert(n == 0)"));
}

TEST_CASE("timer:on_timeout of a non-function is a logged no-op") {
	World world;
	ScriptServer scripts(world);
	Log::Clear();
	CHECK(scripts.DoString("t = timers:create(0.1, true); t:on_timeout(1)"));
	CHECK(logHas("expected a function"));
	scripts.UpdateTimers(0.2f);
}

TEST_CASE("Lua error in on_timeout is logged and does not unwind") {
	World world;
	ScriptServer scripts(world);
	REQUIRE(scripts.DoString(R"(
		t = timers:create(0.1, true)
		t:on_timeout(function() error('timer boom') end)
	)"));
	Log::Clear();
	scripts.UpdateTimers(0.2f);
	CHECK(logHas("timer boom"));
	CHECK(scripts.DoString("assert(true)"));
}

TEST_CASE("timer timeout and repeating properties write back") {
	World world;
	ScriptServer scripts(world);
	REQUIRE(scripts.DoString(R"(
		n = 0
		t = timers:create(2, true)
		t:on_timeout(function() n = n + 1 end)
		t.timeout = 0.5
		t.repeating = true
		assert(t.timeout == 0.5)
		assert(t.repeating == true)
	)"));
	scripts.UpdateTimers(1.f);
	CHECK(scripts.DoString("assert(n == 2)"));
	CHECK(scripts.DoString("assert(t:is_running())"));
}

TEST_CASE("tweens:create auto-starts and reports linear progress") {
	World world;
	ScriptServer scripts(world);
	REQUIRE(scripts.DoString(R"(
		seen = -1
		n = 0
		tw = tweens:create(1)
		tw:on_update(function(t) seen = t end)
		tw:on_finish(function() n = n + 1 end)
		assert(tw:is_running())
		assert(not tw:is_paused())
		assert(not tw:is_finished())
		assert(tw.duration == 1)
	)"));
	scripts.UpdateTweens(0.25f);
	CHECK(scripts.DoString("assert(math.abs(seen - 0.25) < 0.0001)"));
	CHECK(scripts.DoString("assert(math.abs(tw.progress - 0.25) < 0.0001)"));
	CHECK(scripts.DoString("assert(math.abs(tw.remaining - 0.75) < 0.0001)"));
	CHECK(scripts.DoString("assert(n == 0)"));
	scripts.UpdateTweens(0.75f);
	CHECK(scripts.DoString("assert(math.abs(seen - 1) < 0.0001)"));
	CHECK(scripts.DoString("assert(n == 1)"));
	CHECK(scripts.DoString("assert(tw:is_finished())"));
	scripts.UpdateTweens(1.f);
	CHECK(scripts.DoString("assert(n == 1)"));
}

TEST_CASE("tweens:create with auto_start false stays stopped") {
	World world;
	ScriptServer scripts(world);
	REQUIRE(scripts.DoString(R"(
		seen = -1
		tw = tweens:create(0.5, false)
		tw:on_update(function(t) seen = t end)
		assert(not tw:is_running())
	)"));
	scripts.UpdateTweens(0.5f);
	CHECK(scripts.DoString("assert(seen == -1)"));
	CHECK(scripts.DoString("tw:start(); assert(tw:is_running())"));
	scripts.UpdateTweens(0.5f);
	CHECK(scripts.DoString("assert(math.abs(seen - 1) < 0.0001)"));
	CHECK(scripts.DoString("assert(tw:is_finished())"));
}

TEST_CASE("tween pause holds elapsed and stop resets it without finish") {
	World world;
	ScriptServer scripts(world);
	REQUIRE(scripts.DoString(R"(
		n = 0
		tw = tweens:create(1)
		tw:on_finish(function() n = n + 1 end)
	)"));
	scripts.UpdateTweens(0.4f);
	CHECK(scripts.DoString("assert(math.abs(tw.elapsed - 0.4) < 0.0001)"));
	CHECK(scripts.DoString("tw:pause(); assert(tw:is_paused())"));
	scripts.UpdateTweens(0.5f);
	CHECK(scripts.DoString("assert(n == 0)"));
	CHECK(scripts.DoString("assert(math.abs(tw.elapsed - 0.4) < 0.0001)"));
	CHECK(scripts.DoString("tw:start(); assert(tw:is_running())"));
	scripts.UpdateTweens(0.1f);
	CHECK(scripts.DoString("assert(math.abs(tw.elapsed - 0.5) < 0.0001)"));
	CHECK(scripts.DoString("tw:stop(); assert(not tw:is_running()); assert(tw.elapsed == 0)"));
	CHECK(scripts.DoString("assert(not tw:is_finished())"));
	CHECK(scripts.DoString("assert(n == 0)"));
}

TEST_CASE("tween large delta jumps to the end once") {
	World world;
	ScriptServer scripts(world);
	REQUIRE(scripts.DoString(R"(
		updates = 0
		n = 0
		seen = -1
		tw = tweens:create(0.1)
		tw:on_update(function(t)
			updates = updates + 1
			seen = t
		end)
		tw:on_finish(function() n = n + 1 end)
	)"));
	scripts.UpdateTweens(5.f);
	CHECK(scripts.DoString("assert(updates == 1)"));
	CHECK(scripts.DoString("assert(math.abs(seen - 1) < 0.0001)"));
	CHECK(scripts.DoString("assert(n == 1)"));
	CHECK(scripts.DoString("assert(tw:is_finished())"));
}

TEST_CASE("tween:on_update(nil) and on_finish(nil) clear callbacks") {
	World world;
	ScriptServer scripts(world);
	REQUIRE(scripts.DoString(R"(
		seen = -1
		n = 0
		tw = tweens:create(0.1)
		tw:on_update(function(t) seen = t end)
		tw:on_finish(function() n = n + 1 end)
	)"));
	CHECK(scripts.DoString("tw:on_update(nil); tw:on_finish(nil)"));
	scripts.UpdateTweens(0.2f);
	CHECK(scripts.DoString("assert(seen == -1)"));
	CHECK(scripts.DoString("assert(n == 0)"));
	CHECK(scripts.DoString("assert(tw:is_finished())"));
}

TEST_CASE("tween:on_update of a non-function is a logged no-op") {
	World world;
	ScriptServer scripts(world);
	Log::Clear();
	CHECK(scripts.DoString("tw = tweens:create(0.1, false); tw:on_update(1); tw:on_finish(1)"));
	CHECK(logHas("expected a function"));
}

TEST_CASE("Lua error in on_update is logged and does not unwind") {
	World world;
	ScriptServer scripts(world);
	REQUIRE(scripts.DoString(R"(
		tw = tweens:create(0.1)
		tw:on_update(function(t) error('tween boom') end)
	)"));
	Log::Clear();
	scripts.UpdateTweens(0.2f);
	CHECK(logHas("tween boom"));
	CHECK(scripts.DoString("assert(tw:is_finished())"));
}

TEST_CASE("Lua error in on_finish is logged and does not unwind") {
	World world;
	ScriptServer scripts(world);
	REQUIRE(scripts.DoString(R"(
		tw = tweens:create(0.1)
		tw:on_finish(function() error('tween finish boom') end)
	)"));
	Log::Clear();
	scripts.UpdateTweens(0.2f);
	CHECK(logHas("tween finish boom"));
	CHECK(scripts.DoString("assert(true)"));
}

TEST_CASE("tween duration property write back") {
	World world;
	ScriptServer scripts(world);
	REQUIRE(scripts.DoString(R"(
		n = 0
		tw = tweens:create(2)
		tw:on_finish(function() n = n + 1 end)
		tw.duration = 0.5
		assert(tw.duration == 0.5)
	)"));
	scripts.UpdateTweens(0.5f);
	CHECK(scripts.DoString("assert(n == 1)"));
	CHECK(scripts.DoString("assert(tw:is_finished())"));
}

TEST_CASE("tween:ease applies a named curve") {
	World world;
	ScriptServer scripts(world);
	REQUIRE(scripts.DoString(R"(
		seen = -1
		tw = tweens:create(1)
		tw:ease(Easing.QuadIn)
		tw:on_update(function(t) seen = t end)
	)"));
	scripts.UpdateTweens(0.5f);
	CHECK(scripts.DoString("assert(math.abs(seen - 0.25) < 0.0001)"));
	CHECK(scripts.DoString("assert(math.abs(tw.progress - 0.5) < 0.0001)"));
	CHECK(scripts.DoString("tw:ease(nil)"));
	CHECK(scripts.DoString(R"(
		tw:stop()
		tw:start()
		seen = -1
	)"));
	scripts.UpdateTweens(0.5f);
	CHECK(scripts.DoString("assert(math.abs(seen - 0.5) < 0.0001)"));
}

TEST_CASE("tween:ease of a non-number is a logged no-op") {
	World world;
	ScriptServer scripts(world);
	Log::Clear();
	CHECK(scripts.DoString("tw = tweens:create(1, false); tw:ease('nope')"));
	CHECK(logHas("expected an Easing or nil"));
}

TEST_CASE("AudioSource play pause stop are Lua methods") {
	World world;
	Input input;
	FileSystem fs;
	ResourceRegistry resources;
	SceneManager scenes(world, resources, fs);
	ProjectSettings settings;
	ScriptServer scripts(world);
	AudioMixer mixer;
	mixer.Init(world, resources);
	scripts.BindEngine(input, scenes, resources, settings, &mixer);

	Entity entity = world.CreateEntity();
	entity.Add<AudioSource>();
	scripts.SetEntity("e", entity);

	REQUIRE(scripts.DoString(R"(
		local src = e:get("AudioSource")
		assert(src:is_playing() == false)
		assert(src:is_paused() == false)
		src:play()
		assert(src:is_playing())
		src:pause()
		assert(src:is_paused())
		assert(src:is_playing() == false)
		src:play()
		assert(src:is_playing())
		src:stop()
		assert(src:is_playing() == false)
		assert(src:is_paused() == false)
	)"));
	CHECK(mixer.IsPlaying(entity) == false);
}

TEST_CASE("Lua RigidBody2D velocity methods go through PhysicsWorld") {
	World world;
	Input input;
	FileSystem fs;
	ResourceRegistry resources;
	SceneManager scenes(world, resources, fs);
	ProjectSettings settings;
	ScriptServer scripts(world);
	PhysicsWorld physics;
	physics.Init(world);
	scripts.BindEngine(input, scenes, resources, settings, nullptr, &physics);

	Entity entity = world.CreateEntity();
	entity.Add(Transform2D{});
	RigidBody2D body;
	body.type = BodyType2D::Kinematic;
	entity.Add(body);
	physics.Sync();
	scripts.SetEntity("e", entity);

	REQUIRE(scripts.DoString(R"(
		local body = e:get("RigidBody2D")
		body:set_linear_velocity(Vector2(100, 0))
		local v = body:linear_velocity()
		assert(math.abs(v.x - 100) < 0.01)
		assert(math.abs(v.y) < 0.01)
		body:set_angular_velocity(90)
		assert(math.abs(body:angular_velocity() - 90) < 0.01)
		body:apply_force(Vector2(0, 10))
		body:apply_impulse(Vector2(1, 0))
	)"));
	CHECK(physics.GetLinearVelocity(entity).x == doctest::Approx(100.f).epsilon(0.05));
}

TEST_CASE("Lua _collision_begin runs with the other collider") {
	World world;
	Input input;
	FileSystem fs;
	ResourceRegistry resources;
	SceneManager scenes(world, resources, fs);
	ProjectSettings settings;
	ScriptServer scripts(world);
	PhysicsWorld physics;
	physics.Init(world);
	scripts.BindEngine(input, scenes, resources, settings, nullptr, &physics);

	REQUIRE(scripts.DoString(R"(
		local HitLog = world:component("HitLog", {})
		function HitLog:_collision_begin(other)
			self.hits = (self.hits or 0) + 1
			self.otherName = other.name
		end
		function HitLog:_trigger_begin(other)
			self.triggers = (self.triggers or 0) + 1
		end
	)"));

	Entity floor = world.CreateEntity();
	floor.SetName("Floor");
	floor.Add(Transform2D{});
	RigidBody2D floorBody;
	floorBody.type = BodyType2D::Static;
	floor.Add(floorBody);
	Collider2D floorCollider;
	floorCollider.size = Vector2(400.f, 20.f);
	floor.Add(floorCollider);

	Entity box = world.CreateEntity();
	box.SetName("Box");
	box.Add(Transform2D{Vector2(0.f, 200.f), 0.f, Vector2(1.f, 1.f)});
	box.Add(RigidBody2D{});
	Collider2D boxCollider;
	boxCollider.size = Vector2(100.f, 100.f);
	box.Add(boxCollider);
	physics.Sync();
	scripts.SetEntity("e", box);
	REQUIRE(scripts.DoString("e:add('HitLog')"));

	const f32 dt = physics.Config().fixedDelta;
	for (i32 i = 0; i < 90; ++i) {
		physics.Step(dt);
	}

	REQUIRE(scripts.DoString(R"(
		local h = e:get("HitLog")
		assert(h.hits == 1)
		assert(h.otherName == "Floor")
		assert(h.triggers == nil)
	)"));
}

TEST_CASE("Lua _trigger_begin runs for an isTrigger overlap") {
	World world;
	Input input;
	FileSystem fs;
	ResourceRegistry resources;
	SceneManager scenes(world, resources, fs);
	ProjectSettings settings;
	ScriptServer scripts(world);
	PhysicsWorld physics;
	physics.Init(world);
	scripts.BindEngine(input, scenes, resources, settings, nullptr, &physics);

	REQUIRE(scripts.DoString(R"(
		local HitLog = world:component("HitLog", {})
		function HitLog:_collision_begin(other)
			self.hits = (self.hits or 0) + 1
		end
		function HitLog:_trigger_begin(other)
			self.triggers = (self.triggers or 0) + 1
			self.otherName = other.name
		end
	)"));

	Entity floor = world.CreateEntity();
	floor.SetName("Floor");
	floor.Add(Transform2D{});
	RigidBody2D floorBody;
	floorBody.type = BodyType2D::Static;
	floor.Add(floorBody);
	Collider2D trigger;
	trigger.size = Vector2(400.f, 20.f);
	trigger.isTrigger = true;
	floor.Add(trigger);

	Entity box = world.CreateEntity();
	box.SetName("Box");
	box.Add(Transform2D{Vector2(0.f, 200.f), 0.f, Vector2(1.f, 1.f)});
	box.Add(RigidBody2D{});
	Collider2D boxCollider;
	boxCollider.size = Vector2(100.f, 100.f);
	box.Add(boxCollider);
	physics.Sync();
	scripts.SetEntity("e", box);
	REQUIRE(scripts.DoString("e:add('HitLog')"));

	const f32 dt = physics.Config().fixedDelta;
	for (i32 i = 0; i < 90; ++i) {
		physics.Step(dt);
	}

	REQUIRE(scripts.DoString(R"(
		local h = e:get("HitLog")
		assert(h.triggers == 1)
		assert(h.otherName == "Floor")
		assert(h.hits == nil)
	)"));
}

TEST_CASE("Lua command registrations follow module reload and removed sources") {
	const auto dir = makeTempDir();
	FileSystem fs;
	fs.MountFolder("res", dir);
	World world;
	ScriptServer scripts(world);
	writeFile(dir / "commands.lua", R"(
		editor.commands.register { id = "tools.old", title = "Old", run = function() end }
	)");
	REQUIRE(scripts.LoadModules(fs, {"res://commands.lua"}));
	CHECK(scripts.Commands().Search("").size() == 1);
	writeFile(dir / "commands.lua", R"(
		editor.commands.register { id = "tools.new", title = "New", run = function() end }
	)");
	REQUIRE(scripts.ReloadModules(fs));
	CHECK(scripts.Commands().Search("").size() == 1);
	CHECK(scripts.Commands().Search("").at(0).id == "tools.new");
	REQUIRE(scripts.ReloadModules(fs, {}));
	CHECK(scripts.Commands().Search("").empty());
	std::filesystem::remove_all(dir);
}

TEST_CASE("Reload cancels a suspended Lua command before replacing its source") {
	const auto dir = makeTempDir();
	FileSystem fs;
	fs.MountFolder("res", dir);
	World world;
	ScriptServer scripts(world);
	writeFile(dir / "commands.lua", R"(
		editor.commands.register {
			id = "wait", title = "Wait",
			run = function()
				editor.input.text { title = "Wait for input" }
				stale_callback_ran = true
			end,
		}
	)");
	REQUIRE(scripts.LoadModules(fs, {"res://commands.lua"}));
	std::string error;
	REQUIRE(scripts.Commands().Execute("wait", false, error));
	REQUIRE(scripts.CommandInputs().Current());
	REQUIRE(scripts.ReloadModules(fs, {}));
	CHECK_FALSE(scripts.CommandInputs().Current());
	CHECK_FALSE(scripts.CommandInputs().Submit("obsolete"));
	CHECK_FALSE(scripts.Lua().get<sol::optional<bool>>("stale_callback_ran").value_or(false));
	std::filesystem::remove_all(dir);
}

TEST_CASE("Script file events reload required modules once per batch and ignore unrelated files") {
	const auto root = makeTempDir();
	writeFile(root / "value.lua", "return 1\n");
	writeFile(root / "main.lua", "runs = (runs or 0) + 1; n = require('res://./value.lua')\n");
	FileSystem fs;
	fs.MountFolder("res", root);
	World world;
	ScriptServer scripts(world);
	REQUIRE(scripts.LoadModules(fs, {"res://main.lua"}));
	writeFile(root / "value.lua", "return 2\n");
	fs.Events()->changed("res://value.lua");
	fs.Events()->changed("res://value.lua");
	CHECK(scripts.DoString("assert(n == 1 and runs == 1)"));
	REQUIRE(scripts.FlushFileChanges());
	CHECK(scripts.DoString("assert(n == 2 and runs == 2)"));
	fs.Events()->changed("res://unused.lua");
	REQUIRE(scripts.FlushFileChanges());
	fs.Events()->changed("res://value.lua"); // Same bytes: no redundant execution.
	REQUIRE(scripts.FlushFileChanges());
	CHECK(scripts.DoString("assert(runs == 2)"));
	// Removing the dependency from main also drops its logical subscription.
	writeFile(root / "main.lua", "runs = runs + 1; n = 3\n");
	fs.Events()->changed("res://main.lua");
	REQUIRE(scripts.FlushFileChanges());
	writeFile(root / "value.lua", "return 4\n");
	fs.Events()->changed("res://value.lua");
	REQUIRE(scripts.FlushFileChanges());
	CHECK(scripts.DoString("assert(n == 3 and runs == 3)"));
}

TEST_CASE("Script hot reload replaces live hooks and systems while retaining entity fields") {
	const auto root = makeTempDir();
	const auto source = [](int amount, bool removed = false) {
		return std::string("local M = world:component('HotMover', {speed = 3})\n") +
			   "function M:_start() self.speed = self.speed + 10 end\n" +
			   "function M:_update(dt) self.speed = self.speed + " + std::to_string(amount) +
			   " end\n" + (removed ? "" : "function M:removed() end\n") +
			   "world:system('HotSystem'):with('HotMover'):each(function(e, m) ticks = (ticks or "
			   "0) + 1 end)\n";
	};
	writeFile(root / "main.lua", source(1));
	FileSystem fs;
	fs.MountFolder("res", root);
	World world;
	ScriptServer scripts(world);
	REQUIRE(scripts.LoadModules(fs, {"res://main.lua"}));
	Entity entity = world.CreateEntity();
	REQUIRE(scripts.AttachScript(entity, "res://main.lua"));
	world.Progress(0.1f);
	const auto type = world.FindComponent("HotMover");
	const f32 before = *entity.GetField(type, "speed").As<f32>();
	REQUIRE(before == doctest::Approx(14));
	writeFile(root / "main.lua", source(5, true));
	fs.Events()->changed("res://main.lua");
	REQUIRE(scripts.FlushFileChanges());
	CHECK(*entity.GetField(type, "speed").As<f32>() == doctest::Approx(before));
	world.Progress(0.1f);
	CHECK(*entity.GetField(type, "speed").As<f32>() == doctest::Approx(before + 5));
	CHECK(scripts.DoString("assert(ticks == 2)"));
	CHECK_FALSE(scripts.CallComponentMethod(entity, "HotMover", "removed"));
}

TEST_CASE("Script hot reload keeps running code on syntax errors and recovers on a later save") {
	const auto root = makeTempDir();
	writeFile(root / "helper.lua", "return 1\n");
	writeFile(root / "main.lua",
		"n = require('res://helper.lua'); function value() return n end\n");
	FileSystem fs;
	fs.MountFolder("res", root);
	World world;
	ScriptServer scripts(world);
	REQUIRE(scripts.LoadModules(fs, {"res://main.lua"}));
	writeFile(root / "helper.lua", "return function(\n");
	fs.Events()->changed("res://helper.lua");
	CHECK_FALSE(scripts.FlushFileChanges());
	CHECK(scripts.DoString("assert(value() == 1)"));
	writeFile(root / "helper.lua", "return 7\n");
	fs.Events()->changed("res://helper.lua");
	REQUIRE(scripts.FlushFileChanges());
	CHECK(scripts.DoString("assert(value() == 7)"));
}

TEST_CASE(
	"Script reload subscriptions survive failed new requires and stop after module list removal") {
	const auto root = makeTempDir();
	writeFile(root / "main.lua", "n = require('res://missing.lua')\n");
	FileSystem fs;
	fs.MountFolder("res", root);
	World world;
	ScriptServer scripts(world);
	CHECK_FALSE(scripts.LoadModules(fs, {"res://main.lua"}));
	writeFile(root / "missing.lua", "return 8\n");
	fs.Events()->changed("res://missing.lua");
	REQUIRE(scripts.FlushFileChanges());
	CHECK(scripts.DoString("assert(n == 8)"));
	REQUIRE(scripts.ReloadModules(fs, {}));
	writeFile(root / "main.lua", "n = 0\n");
	fs.Events()->changed("res://main.lua");
	REQUIRE(scripts.FlushFileChanges());
	CHECK(scripts.DoString("assert(n == 8)"));
}

TEST_CASE("Direct DoFile sources receive script hot reload and unsubscribe safely") {
	const auto root = makeTempDir();
	writeFile(root / "direct.lua", "n = 1\n");
	FileSystem fs;
	fs.MountFolder("res", root);
	World world;
	{
		ScriptServer scripts(world);
		REQUIRE(scripts.DoFile(fs, "res://direct.lua"));
		writeFile(root / "direct.lua", "n = 2\n");
		fs.Events()->changed("res://direct.lua");
		REQUIRE(scripts.FlushFileChanges());
		CHECK(scripts.DoString("assert(n == 2)"));
	}
	CHECK_NOTHROW(fs.Events()->changed("res://direct.lua"));
}

TEST_CASE("Lua queries project input actions") {
	LuaEngine engine;
	engine.input.SetActions({{"left", {{Key::A}}}, {"right", {{Key::D}}}, {"down", {{Key::S}}},
		{"up", {{Key::W}}}, {"save", {{Key::S, InputModifierControl}}}});

	engine.input.HandleKey(Key::D, KeyAction::Press);
	engine.input.HandleKey(Key::W, KeyAction::Press);
	CHECK(engine.scripts.DoString(R"(
		assert(input:is_action_pressed("right"))
		assert(input:is_action_just_pressed("right"))
		assert(not input:is_action_pressed("left"))
		assert(not input:is_action_pressed("missing"))
		assert(input:get_action_strength("right") == 1)
		assert(input:get_axis("left", "right") == 1)
		local v = input:get_vector("left", "right", "down", "up")
		assert(v.x == 1 and v.y == 1)
		assert(not input:is_action_pressed("save"))
	)"));

	engine.input.Update();
	engine.input.HandleKey(Key::D, KeyAction::Release);
	CHECK(engine.scripts.DoString(R"(
		assert(input:is_action_just_released("right"))
		assert(input:get_axis("left", "right") == 0)
	)"));
}

TEST_CASE("Script errors name the file by its res:// path") {
	const auto root = makeTempDir();
	writeFile(root / "bad.lua", "local x = 1\nerror('nope')\n");
	FileSystem fs;
	fs.MountFolder("res", root);
	World world;
	ScriptServer scripts(world);
	Log::Clear();
	CHECK_FALSE(scripts.DoFile(fs, "res://bad.lua"));
	CHECK(logHas("res://bad.lua:2: nope"));
	CHECK_FALSE(logHas("[string"));
	// The traceback starts at the code that raised the error; the message
	// handler itself (sol's "sol.\xF0\x9F\x94\xA9" global) is not a frame.
	CHECK(logHas(
		"stack traceback:\n\t[C]: in function 'base.error'\n\tres://bad.lua:2: in main chunk"));
	CHECK_FALSE(logHas("sol."));
}

TEST_CASE("The debugger stops in a component's _update and shows its fields") {
	const auto root = makeTempDir();
	writeFile(root / "mover.lua", "local Mover = world:component('Mover', { speed = 1 })\n"
								  "function Mover:_update(dt)\n"
								  "  self.speed = self.speed + 1\n"
								  "end\n");
	FileSystem fs;
	fs.MountFolder("res", root);
	World world;
	ScriptServer scripts(world);
	REQUIRE(scripts.LoadModules(fs, {"res://mover.lua"}));
	REQUIRE(scripts.DoString("e = world:create_entity('Guy') e:add('Mover')"));

	LuaDebugger &debugger = scripts.Debugger();
	LuaBreakpoint breakpoint;
	breakpoint.line = 3;
	debugger.SetBreakpoints("res://mover.lua", {breakpoint});
	int stops = 0;
	debugger.SetStopHandler([&](const LuaStopEvent &event) {
		++stops;
		CHECK(event.source == "res://mover.lua");
		CHECK(event.line == 3);

		i32 locals = 0;
		for (const LuaScope &scope : debugger.Scopes(0)) {
			if (scope.name == "Locals") {
				locals = scope.reference;
			}
		}
		i32 self = 0;
		for (const LuaVariable &variable : debugger.Variables(locals)) {
			if (variable.name == "self") {
				self = variable.reference;
			}
		}
		REQUIRE(self != 0);

		// The component view expands to its reflected fields and entity.
		i32 entity = 0;
		bool sawSpeed = false;
		for (const LuaVariable &field : debugger.Variables(self)) {
			if (field.name == "speed") {
				sawSpeed = true;
				CHECK(field.value == "1");
			}
			if (field.name == "entity") {
				entity = field.reference;
			}
		}
		CHECK(sawSpeed);
		REQUIRE(entity != 0);

		i32 components = 0;
		bool sawName = false;
		for (const LuaVariable &field : debugger.Variables(entity)) {
			if (field.name == "name") {
				sawName = true;
				CHECK(field.value == "\"Guy\"");
			}
			if (field.name == "components") {
				components = field.reference;
			}
		}
		CHECK(sawName);
		REQUIRE(components != 0);
		bool sawMover = false;
		for (const LuaVariable &component : debugger.Variables(components)) {
			sawMover = sawMover || component.name == "Mover";
		}
		CHECK(sawMover);

		// Assigning a child writes through the component's __newindex.
		const LuaEvalResult set = debugger.SetVariable(self, "speed", "41");
		CHECK(set.success);
		CHECK(set.value == "41");
		return LuaResumeAction::Continue;
	});

	world.Progress(1.f / 60.f);
	CHECK(stops == 1);
	CHECK(scripts.DoString("assert(e:get('Mover').speed == 42)"));
	debugger.SetStopHandler({});
}

TEST_CASE("Break on error stops in a component hook run by the engine") {
	const auto root = makeTempDir();
	writeFile(root / "broken.lua", "local Broken = world:component('Broken', {})\n"
								   "function Broken:_update(dt)\n"
								   "  local missing = nil\n"
								   "  return missing.field\n"
								   "end\n");
	FileSystem fs;
	fs.MountFolder("res", root);
	World world;
	ScriptServer scripts(world);
	REQUIRE(scripts.LoadModules(fs, {"res://broken.lua"}));
	REQUIRE(scripts.DoString("e = world:create_entity() e:add('Broken')"));

	LuaDebugger &debugger = scripts.Debugger();
	debugger.SetBreakOnError(true);
	std::vector<LuaStopEvent> stops;
	debugger.SetStopHandler([&](const LuaStopEvent &event) {
		stops.push_back(event);
		return LuaResumeAction::Continue;
	});
	Log::Clear();
	world.Progress(1.f / 60.f);
	REQUIRE(stops.size() == 1);
	CHECK(stops[0].reason == LuaStopReason::Exception);
	CHECK(stops[0].source == "res://broken.lua");
	CHECK(stops[0].line == 4);
	// The engine still reports the error as before.
	CHECK(logHas("res://broken.lua:4"));
	debugger.SetStopHandler({});
}

TEST_CASE("tostring of a scene names it") {
	LuaEngine engine;
	Scene scene = engine.scenes.Create("Level");
	REQUIRE(engine.scenes.SetCurrent(scene));
	REQUIRE(engine.scripts.DoString(R"lua(
		level = scenes:current()
		assert(tostring(level) == "Scene(Level)", tostring(level))
	)lua"));
	engine.scenes.Close(scene);
	engine.scenes.Flush();
	REQUIRE(engine.scripts.DoString(R"lua(
		assert(tostring(level) == "Scene(<invalid>)", tostring(level))
	)lua"));
}
