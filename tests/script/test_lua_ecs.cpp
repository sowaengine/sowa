#include <string>

#include <doctest/doctest.h>

#include "core/log.hpp"
#include "ecs/entity.hpp"
#include "ecs/script_tags.hpp"
#include "ecs/world.hpp"
#include "script/script_server.hpp"

using namespace sowa;

TEST_CASE("world:component registers a data-described type Lua can write") {
	World world;
	ScriptServer scripts(world);
	REQUIRE(scripts.DoString(R"(
		local Health = world:component("Health", { hp = 100 })
		e = world:create_entity("Guy")
		e:add("Health")
		assert(e:get("Health").hp == 100)
		e:get("Health").hp = 40
	)"));
	Entity guy = world.Find("Guy");
	REQUIRE(guy.IsAlive());
	ComponentType health = world.FindComponent("Health");
	REQUIRE(health.IsValid());
	f32 hp = 0.f;
	REQUIRE(guy.GetField(health, "hp").Get(hp));
	CHECK(hp == doctest::Approx(40.f));
}

TEST_CASE("Lua _update is a play-only system over the component") {
	World world;
	ScriptServer scripts(world);
	REQUIRE(scripts.DoString(R"(
		local Player = world:component("Player", { speed = 1 })
		function Player:_update(dt)
			self.speed = self.speed + dt
		end
		e = world:create_entity()
		e:add("Player")
	)"));

	world.SetPlayOnlySystemsEnabled(false);
	world.Progress(2.f);
	REQUIRE(scripts.DoString("assert(e:get('Player').speed == 1)"));

	world.SetPlayOnlySystemsEnabled(true);
	world.Progress(2.f);
	REQUIRE(scripts.DoString("assert(e:get('Player').speed == 3)"));
}

TEST_CASE("_exit runs without _start once the instance is live") {
	World world;
	ScriptServer scripts(world);
	REQUIRE(scripts.DoString(R"(
		exited = 0
		local Buff = world:component("Buff", { amount = 1 })
		function Buff:_exit()
			exited = exited + 1
		end
		e = world:create_entity()
		e:add("Buff")
	)"));

	world.SetPlayOnlySystemsEnabled(false);
	world.Progress(0.f);
	REQUIRE(scripts.DoString("e:remove('Buff')"));
	REQUIRE(scripts.DoString("assert(exited == 0)"));

	REQUIRE(scripts.DoString("e:add('Buff')"));
	world.SetPlayOnlySystemsEnabled(true);
	world.Progress(0.f);
	REQUIRE(scripts.DoString("e:remove('Buff')"));
	REQUIRE(scripts.DoString("assert(exited == 1)"));
}

TEST_CASE("Lua methods are reachable through the component view") {
	World world;
	ScriptServer scripts(world);
	REQUIRE(scripts.DoString(R"(
		local Door = world:component("Door", { open = false })
		function Door:unlock()
			self.open = true
		end
		e = world:create_entity()
		e:add("Door")
		e:get("Door"):unlock()
		assert(e:get("Door").open == true)
	)"));
}

TEST_CASE("world:system with two components matches the intersection") {
	World world;
	ScriptServer scripts(world);
	REQUIRE(scripts.DoString(R"(
		world:component("Health", { hp = 10 })
		world:component("Poison", { dps = 3 })
		world:system("PoisonDamage"):with("Health", "Poison"):each(function(e, health, poison)
			health.hp = health.hp - poison.dps
		end)
		sick = world:create_entity()
		sick:add("Health")
		sick:add("Poison")
		well = world:create_entity()
		well:add("Health")
	)"));
	world.Progress(1.f);
	REQUIRE(scripts.DoString("assert(sick:get('Health').hp == 7)"));
	REQUIRE(scripts.DoString("assert(well:get('Health').hp == 10)"));
}

TEST_CASE("Redefining a Lua component migrates fields by name") {
	World world;
	ScriptServer scripts(world);
	REQUIRE(scripts.DoString(R"(
		world:component("Player", { speed = 200, lives = 3 })
		e = world:create_entity()
		e:add("Player")
		e:get("Player").speed = 350
		e:get("Player").lives = 1
		e:get("Player")._timer = 9
		world:component("Player", { jump = 12, speed = 200 })
		assert(e:get("Player").speed == 350)
		assert(e:get("Player").jump == 12)
		assert(e:get("Player").lives == nil)
		assert(e:get("Player")._timer == 9)
	)"));
}

TEST_CASE("An empty Lua component is a tag, not a Find()-able entity") {
	World world;
	ScriptServer scripts(world);
	REQUIRE(scripts.DoString(R"(
		world:component("Game", {})
		assert(world:find("Game") == nil)
		e = world:create_entity("Hero")
		e:add("Game")
		assert(e:has("Game"))
	)"));
	CHECK_FALSE(world.Find("Game").IsAlive());
	CHECK(world.FindComponent("Game").IsValid());
}

TEST_CASE("_start does not re-fire while ScriptLive is set") {
	World world;
	ScriptServer scripts(world);
	REQUIRE(scripts.DoString(R"(
		started = 0
		local Player = world:component("Player", { speed = 1 })
		function Player:_start()
			started = started + 1
		end
		e = world:create_entity()
		e:add("Player")
	)"));
	world.Progress(0.f);
	REQUIRE(scripts.DoString("assert(started == 1)"));
	REQUIRE(scripts.DoString(R"(
		local Player = world:component("Player", { speed = 1 })
		function Player:_start()
			started = started + 1
		end
	)"));
	world.Progress(0.f);
	REQUIRE(scripts.DoString("assert(started == 1)"));
}

TEST_CASE("entity:add from a system applies Lua field defaults") {
	World world;
	ScriptServer scripts(world);
	REQUIRE(scripts.DoString(R"(
		world:component("Marker", {})
		world:component("Health", { hp = 100 })
		world:system("Grant"):with("Marker"):without("Health"):each(function(e)
			e:add("Health")
			assert(e:get("Health").hp == 100)
		end)
		e = world:create_entity("Guy")
		e:add("Marker")
	)"));
	world.Progress(0.f);
	REQUIRE(scripts.DoString("assert(e:get('Health').hp == 100)"));
	Entity guy = world.Find("Guy");
	REQUIRE(guy.IsAlive());
	f32 hp = 0.f;
	REQUIRE(guy.GetField(world.FindComponent("Health"), "hp").Get(hp));
	CHECK(hp == doctest::Approx(100.f));
}

TEST_CASE("tostring of a component names its type and entity") {
	World world;
	ScriptServer scripts(world);
	REQUIRE(scripts.DoString(R"lua(
		world:component("Health", { hp = 100 })
		local parent = world:create_entity("Party")
		e = world:create_entity("Guy")
		e.parent = parent
		e:add("Health")
		health = e:get("Health")
		assert(tostring(health) == "Health(Party/Guy)", tostring(health))
		e:remove("Health")
		assert(tostring(health) == "Health(<removed>)", tostring(health))
		e:destroy()
		assert(tostring(health) == "Health(<invalid entity>)", tostring(health))
	)lua"));
}
