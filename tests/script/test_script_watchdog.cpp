#include <doctest/doctest.h>

#include <chrono>
#include <string>

#include <sol/sol.hpp>

#include "script/lua_debugger.hpp"
#include "script/script_watchdog.hpp"

using namespace sowa;
using namespace std::chrono_literals;

TEST_CASE("The watchdog fires once when no beat arrives within the timeout") {
	int fired = 0;
	ScriptWatchdog watchdog(3s, [&] { ++fired; });
	watchdog.SetArmed(true);
	watchdog.Beat();
	const auto start = ScriptWatchdog::Clock::now();

	watchdog.Check(start + 2s);
	CHECK(fired == 0);
	watchdog.Check(start + 3s + 1ms);
	CHECK(fired == 1);
	CHECK(watchdog.HasFired());
	// Once per stall: it does not fire again until the main thread beats.
	watchdog.Check(start + 10s);
	CHECK(fired == 1);

	CHECK(watchdog.Beat());
	CHECK_FALSE(watchdog.HasFired());
	CHECK_FALSE(watchdog.Beat());
	const auto later = ScriptWatchdog::Clock::now();
	watchdog.Check(later + 1s);
	CHECK(fired == 1);
	watchdog.Check(later + 4s);
	CHECK(fired == 2);
}

TEST_CASE("The timeout counts from the latest beat") {
	int fired = 0;
	ScriptWatchdog watchdog(3s, [&] { ++fired; });
	watchdog.SetArmed(true);
	const auto start = ScriptWatchdog::Clock::now();
	watchdog.Beat(start);
	watchdog.Beat(start + 10s);
	watchdog.Check(start + 12s);
	CHECK(fired == 0);
	watchdog.Check(start + 13s);
	CHECK(fired == 1);
}

TEST_CASE("A disarmed watchdog never fires") {
	int fired = 0;
	ScriptWatchdog watchdog(3s, [&] { ++fired; });
	watchdog.Beat();
	watchdog.Check(ScriptWatchdog::Clock::now() + 1h);
	CHECK(fired == 0);
	watchdog.SetArmed(true);
	watchdog.Check(ScriptWatchdog::Clock::now() + 1h);
	CHECK(fired == 1);
}

TEST_CASE("A cancelled pause request is not taken by the next script") {
	sol::state lua;
	lua.open_libraries(sol::lib::base);
	LuaDebugger debugger(lua.lua_state());
	int stops = 0;
	debugger.SetStopHandler([&](const LuaStopEvent &) {
		++stops;
		return LuaResumeAction::Continue;
	});
	debugger.SetPauseWatch(true);
	debugger.RequestPause();
	debugger.CancelPause();
	REQUIRE(lua.safe_script("local n = 0 for i = 1, 100000 do n = n + i end").valid());
	CHECK(stops == 0);

	debugger.RequestPause();
	REQUIRE(lua.safe_script("local n = 0 for i = 1, 100000 do n = n + i end").valid());
	CHECK(stops == 1);
}

TEST_CASE("The watchdog thread stops an endless script loop") {
	sol::state lua;
	lua.open_libraries(sol::lib::base);
	LuaDebugger debugger(lua.lua_state());
	debugger.SetPauseWatch(true);
	LuaStopEvent stop;
	int stops = 0;
	debugger.SetStopHandler([&](const LuaStopEvent &event) {
		stop = event;
		++stops;
		return LuaResumeAction::Abort;
	});
	REQUIRE(
		lua.safe_script("function spin()\n"
						"  local i = 0\n"
						"  while true do i = i + 1 end\n"
						"end\n",
			   sol::script_pass_on_error, "@res://spin.lua")
			.valid());

	ScriptWatchdog watchdog(100ms, [&] { debugger.RequestPause(); });
	watchdog.SetArmed(true);
	watchdog.Start();
	sol::protected_function spin = lua["spin"];
	const sol::protected_function_result result = spin();
	watchdog.Stop();

	REQUIRE_FALSE(result.valid());
	CHECK(std::string(result.get<sol::error>().what()).find(LuaDebugger::kAbortMessage) !=
		  std::string::npos);
	CHECK(stops == 1);
	CHECK(stop.reason == LuaStopReason::Pause);
	CHECK(stop.source == "res://spin.lua");
	CHECK(stop.line == 3);
	CHECK(watchdog.HasFired());
}
