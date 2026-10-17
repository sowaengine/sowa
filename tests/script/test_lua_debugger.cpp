#include <doctest/doctest.h>

#include <chrono>
#include <deque>
#include <functional>
#include <initializer_list>
#include <string>
#include <thread>
#include <vector>

#include <sol/sol.hpp>

#include "script/lua_debugger.hpp"

using namespace sowa;

namespace {

// The debugger is made after the libraries, as ScriptServer makes it.
struct LuaWithLibraries {
	sol::state lua;

	LuaWithLibraries() {
		lua.open_libraries(sol::lib::base, sol::lib::coroutine, sol::lib::string, sol::lib::table);
	}
};

// A Lua state with a debugger attached the way ScriptServer attaches it:
// the debugger's message handler as sol's default, for break-on-error.
struct DebugFixture : LuaWithLibraries {
	LuaDebugger debugger{lua.lua_state()};
	std::vector<LuaStopEvent> stops;
	// Run during each stop, then popped; Continue once empty.
	std::deque<std::function<LuaResumeAction(LuaDebugger &)>> onStop;
	std::vector<std::string> output;

	DebugFixture() {
		lua_State *L = lua.lua_state();
		debugger.PushErrorHandler(L);
		lua_setglobal(L, sol::detail::default_handler_name());

		debugger.SetStopHandler([this](const LuaStopEvent &event) {
			stops.push_back(event);
			if (onStop.empty()) {
				return LuaResumeAction::Continue;
			}
			auto next = std::move(onStop.front());
			onStop.pop_front();
			return next(debugger);
		});
		debugger.SetOutputHandler([this](std::string_view text) { output.emplace_back(text); });
	}

	// Loads lines as res://<name>; line N of the file is lines[N - 1].
	void Load(const std::string &name, std::initializer_list<const char *> lines) {
		std::string code;
		for (const char *line : lines) {
			code += line;
			code += '\n';
		}
		const auto result = lua.safe_script(code, sol::script_pass_on_error, "@res://" + name);
		REQUIRE(result.valid());
	}

	sol::protected_function_result Call(const char *function, auto &&...args) {
		sol::protected_function fn = lua[function];
		return fn(std::forward<decltype(args)>(args)...);
	}

	void Then(LuaResumeAction action) {
		onStop.push_back([action](LuaDebugger &) { return action; });
	}

	void Then(std::function<LuaResumeAction(LuaDebugger &)> fn) {
		onStop.push_back(std::move(fn));
	}

	std::vector<i32> Lines() const {
		std::vector<i32> lines;
		for (const LuaStopEvent &stop : stops) {
			lines.push_back(stop.line);
		}
		return lines;
	}
};

LuaBreakpoint At(i32 line) {
	LuaBreakpoint breakpoint;
	breakpoint.line = line;
	return breakpoint;
}

const LuaVariable *Find(const std::vector<LuaVariable> &variables, std::string_view name) {
	for (const LuaVariable &variable : variables) {
		if (variable.name == name) {
			return &variable;
		}
	}
	return nullptr;
}

i32 ScopeRef(LuaDebugger &debugger, i32 frame, std::string_view name) {
	for (const LuaScope &scope : debugger.Scopes(frame)) {
		if (scope.name == name) {
			return scope.reference;
		}
	}
	return 0;
}

int LineMaskProbe(lua_State *L) {
	lua_pushboolean(L, (lua_gethookmask(L) & LUA_MASKLINE) != 0);
	return 1;
}

} // namespace

TEST_CASE("A breakpoint stops at its line every time it is reached") {
	DebugFixture f;
	f.Load("tick.lua", {
						   "function tick(n)",				   // 1
						   "  local doubled = n * 2",		   // 2
						   "  total = (total or 0) + doubled", // 3
						   "  return doubled",				   // 4
						   "end",							   // 5
					   });
	f.debugger.SetBreakpoints("res://tick.lua", {At(3)});

	f.Then([](LuaDebugger &debugger) {
		const std::vector<LuaStackFrame> frames = debugger.StackTrace();
		REQUIRE_FALSE(frames.empty());
		CHECK(frames[0].name == "tick");
		CHECK(frames[0].source == "res://tick.lua");
		CHECK(frames[0].line == 3);
		CHECK(debugger.IsStopped());
		return LuaResumeAction::Continue;
	});
	CHECK(f.Call("tick", 2).valid());
	CHECK(f.Call("tick", 5).valid());

	REQUIRE(f.stops.size() == 2);
	for (const LuaStopEvent &stop : f.stops) {
		CHECK(stop.reason == LuaStopReason::Breakpoint);
		CHECK(stop.source == "res://tick.lua");
		CHECK(stop.line == 3);
	}
	CHECK(f.lua.get<int>("total") == 14);
	CHECK_FALSE(f.debugger.IsStopped());
}

TEST_CASE("The hook exists only while breakpoints do, on every thread") {
	DebugFixture f;
	lua_State *L = f.lua.lua_state();
	f.Load("co.lua", {"co = coroutine.create(function() coroutine.yield() end)"});
	lua_State *co = f.lua.get<sol::thread>("co").thread_state();
	REQUIRE(co != nullptr);
	CHECK(lua_gethook(L) == nullptr);
	CHECK(lua_gethook(co) == nullptr);

	f.debugger.SetBreakpoints("res://co.lua", {At(1)});
	CHECK(lua_gethook(L) != nullptr);
	CHECK(lua_gethook(co) != nullptr);

	f.debugger.SetBreakpoints("res://co.lua", {});
	CHECK(lua_gethook(L) == nullptr);
	CHECK(lua_gethook(co) == nullptr);

	f.debugger.SetBreakpoints("res://co.lua", {At(1)});
	f.debugger.ClearAllBreakpoints();
	CHECK(lua_gethook(L) == nullptr);
}

TEST_CASE("Line events run only in functions from sources with breakpoints") {
	DebugFixture f;
	f.lua.set_function("probe", &LineMaskProbe);
	f.Load("watched.lua", {
							  "function watched()", // 1
							  "  return probe()",	// 2
							  "end",				// 3
							  "function unused()",	// 4
							  "  return 0",			// 5
							  "end",				// 6
						  });
	f.Load("other.lua", {"function other() return probe() end"});
	f.debugger.SetBreakpoints("res://watched.lua", {At(5)});

	CHECK(f.Call("watched").get<bool>());
	CHECK_FALSE(f.Call("other").get<bool>());
	CHECK(f.stops.empty());
}

TEST_CASE("Conditions and hit conditions filter breakpoint stops") {
	DebugFixture f;
	f.Load("loop.lua", {
						   "function run()",	 // 1
						   "  for i = 1, 10 do", // 2
						   "    hit = i",		 // 3
						   "  end",				 // 4
						   "end",				 // 5
					   });
	const auto stoppedAt = [&](LuaBreakpoint breakpoint) {
		breakpoint.line = 3;
		f.stops.clear();
		std::vector<std::string> values;
		f.debugger.SetBreakpoints("res://loop.lua", {breakpoint});
		for (int i = 0; i < 10; ++i) {
			f.Then([&values](LuaDebugger &debugger) {
				values.push_back(debugger.Evaluate(0, "i").value);
				return LuaResumeAction::Continue;
			});
		}
		CHECK(f.Call("run").valid());
		f.onStop.clear();
		return values;
	};

	LuaBreakpoint even;
	even.condition = "i % 2 == 0";
	CHECK(stoppedAt(even) == std::vector<std::string>{"2", "4", "6", "8", "10"});

	LuaBreakpoint fromEighth;
	fromEighth.hitCondition = ">= 8";
	CHECK(stoppedAt(fromEighth) == std::vector<std::string>{"8", "9", "10"});

	LuaBreakpoint bare;
	bare.hitCondition = "9";
	CHECK(stoppedAt(bare) == std::vector<std::string>{"9", "10"});

	LuaBreakpoint everyThird;
	everyThird.hitCondition = "% 3";
	CHECK(stoppedAt(everyThird) == std::vector<std::string>{"3", "6", "9"});

	LuaBreakpoint fourth;
	fourth.hitCondition = "== 4";
	CHECK(stoppedAt(fourth) == std::vector<std::string>{"4"});

	// Hits count only when the condition passes: the 2nd even i is 4.
	LuaBreakpoint secondEven;
	secondEven.condition = "i % 2 == 0";
	secondEven.hitCondition = "== 2";
	CHECK(stoppedAt(secondEven) == std::vector<std::string>{"4"});

	// A condition that cannot be evaluated stops, and says why.
	LuaBreakpoint broken;
	broken.condition = "i +";
	CHECK(stoppedAt(broken).size() == 10);
	CHECK_FALSE(f.output.empty());
}

TEST_CASE("Logpoints write interpolated messages without stopping") {
	DebugFixture f;
	f.Load("log.lua", {
						  "function run()",				// 1
						  "  for i = 1, 3 do",			// 2
						  "    local label = 'n' .. i", // 3
						  "    hit = label",			// 4
						  "  end",						// 5
						  "end",						// 6
					  });
	LuaBreakpoint logpoint = At(4);
	logpoint.logMessage = "i={i} label={label} sum={i + 10}";
	f.debugger.SetBreakpoints("res://log.lua", {logpoint});
	CHECK(f.Call("run").valid());
	CHECK(f.stops.empty());
	CHECK(f.output == std::vector<std::string>{"i=1 label=n1 sum=11", "i=2 label=n2 sum=12",
						  "i=3 label=n3 sum=13"});
}

TEST_CASE("Stepping into, over and out follows the call depth") {
	DebugFixture f;
	f.Load("step.lua", {
						   "function inner(v)",	   // 1
						   "  local w = v + 1",	   // 2
						   "  return w",		   // 3
						   "end",				   // 4
						   "function outer()",	   // 5
						   "  local a = inner(1)", // 6
						   "  local b = a + 1",	   // 7
						   "  return b",		   // 8
						   "end",				   // 9
					   });
	f.debugger.SetBreakpoints("res://step.lua", {At(6)});

	SUBCASE("in, over, out, over, over") {
		for (int run = 0; run < 2; ++run) {
			f.stops.clear();
			f.Then(LuaResumeAction::StepIn);
			f.Then(LuaResumeAction::StepOver);
			f.Then(LuaResumeAction::StepOut);
			f.Then(LuaResumeAction::StepOver);
			f.Then(LuaResumeAction::StepOver);
			CHECK(f.Call("outer").get<int>() == 3);
			CHECK(f.Lines() == std::vector<i32>{6, 2, 3, 7, 8});
			CHECK(f.stops[1].reason == LuaStopReason::Step);
		}
	}

	SUBCASE("over skips the call") {
		f.Then(LuaResumeAction::StepOver);
		f.Then(LuaResumeAction::StepOver);
		CHECK(f.Call("outer").valid());
		CHECK(f.Lines() == std::vector<i32>{6, 7, 8});
	}

	SUBCASE("out of the outermost frame continues and removes the line hook") {
		f.Then(LuaResumeAction::StepOut);
		CHECK(f.Call("outer").valid());
		CHECK(f.Lines() == std::vector<i32>{6});
		f.debugger.ClearAllBreakpoints();
		CHECK(lua_gethook(f.lua.lua_state()) == nullptr);
	}
}

TEST_CASE("Break on error stops in the erroring frame before the stack unwinds") {
	DebugFixture f;
	f.Load("fail.lua", {
						   "function fail(x)",								// 1
						   "  local reason = 'bad ' .. x",					// 2
						   "  error(reason)",								// 3
						   "end",											// 4
						   "function go() fail('input') end",				// 5
						   "function caught() return pcall(fail, 'x') end", // 6
					   });

	SUBCASE("off by default") {
		CHECK_FALSE(f.Call("go").valid());
		CHECK(f.stops.empty());
	}

	SUBCASE("on") {
		f.debugger.SetBreakOnError(true);
		f.Then([](LuaDebugger &debugger) {
			const std::vector<LuaStackFrame> frames = debugger.StackTrace();
			REQUIRE(frames.size() >= 2);
			CHECK(frames[0].name == "fail");
			CHECK(frames[0].line == 3);
			CHECK(frames[1].name == "go");
			CHECK(debugger.Evaluate(0, "reason").value == "\"bad input\"");
			// Stepping is ignored: the error is already unwinding.
			return LuaResumeAction::StepIn;
		});
		const auto result = f.Call("go");
		REQUIRE_FALSE(result.valid());
		CHECK(std::string(result.get<sol::error>().what()).find("bad input") != std::string::npos);
		REQUIRE(f.stops.size() == 1);
		CHECK(f.stops[0].reason == LuaStopReason::Exception);
		CHECK(f.stops[0].source == "res://fail.lua");
		CHECK(f.stops[0].line == 3);
		CHECK(f.stops[0].message.find("bad input") != std::string::npos);

		// An error a script catches itself is not a stop, and the ignored
		// step left nothing behind.
		CHECK(f.Call("caught").valid());
		CHECK(f.stops.size() == 1);
		CHECK(lua_gethook(f.lua.lua_state()) == nullptr);
	}
}

TEST_CASE("Variables list locals, varargs, upvalues, globals and table children") {
	DebugFixture f;
	f.Load("vars.lua", {
						   "local up = 10",												 // 1
						   "function show(...)",										 // 2
						   "  local t = { 1, 2, name = 'x', nested = { deep = true } }", // 3
						   "  local s = 'hi'",											 // 4
						   "  local kind = type(s)", // 5: reads a global, so show has _ENV
						   "  return up + #t",		 // 6
						   "end",					 // 7
					   });
	f.debugger.SetBreakpoints("res://vars.lua", {At(6)});
	f.Then([](LuaDebugger &debugger) {
		const std::vector<LuaScope> scopes = debugger.Scopes(0);
		REQUIRE(scopes.size() == 3);
		CHECK(scopes[0].name == "Locals");
		CHECK(scopes[1].name == "Upvalues");
		CHECK(scopes[2].name == "Globals");

		const std::vector<LuaVariable> locals = debugger.Variables(scopes[0].reference);
		const LuaVariable *t = Find(locals, "t");
		REQUIRE(t != nullptr);
		CHECK(t->type == "table");
		CHECK(t->value == "table {4}");
		CHECK(t->reference != 0);
		REQUIRE(Find(locals, "s") != nullptr);
		CHECK(Find(locals, "s")->value == "\"hi\"");
		REQUIRE(Find(locals, "(vararg 1)") != nullptr);
		CHECK(Find(locals, "(vararg 1)")->value == "\"a\"");
		REQUIRE(Find(locals, "(vararg 2)") != nullptr);
		CHECK(Find(locals, "(vararg 2)")->value == "7");

		const std::vector<LuaVariable> children = debugger.Variables(t->reference);
		REQUIRE(children.size() == 4);
		CHECK(children[0].name == "[1]");
		CHECK(children[1].name == "[2]");
		CHECK(children[2].name == "name");
		CHECK(children[2].value == "\"x\"");
		CHECK(children[3].name == "nested");
		const std::vector<LuaVariable> nested = debugger.Variables(children[3].reference);
		REQUIRE(nested.size() == 1);
		CHECK(nested[0].name == "deep");
		CHECK(nested[0].value == "true");

		const std::vector<LuaVariable> upvalues = debugger.Variables(scopes[1].reference);
		REQUIRE(Find(upvalues, "up") != nullptr);
		CHECK(Find(upvalues, "up")->value == "10");
		CHECK(Find(upvalues, "_ENV") == nullptr);

		const std::vector<LuaVariable> globals = debugger.Variables(scopes[2].reference);
		REQUIRE(Find(globals, "show") != nullptr);
		CHECK(Find(globals, "show")->value == "function res://vars.lua:2");
		return LuaResumeAction::Continue;
	});
	CHECK(f.Call("show", "a", 7).get<int>() == 12);
	CHECK(f.stops.size() == 1);
}

TEST_CASE("Evaluate and SetVariable read and change the stopped frame") {
	DebugFixture f;
	f.Load("eval.lua", {
						   "local up = 10",				  // 1
						   "function show()",			  // 2
						   "  local t = { name = 'x' }",  // 3
						   "  local s = 'hi'",			  // 4
						   "  return up + #s, s, t.name", // 5
						   "end",						  // 6
					   });
	f.debugger.SetBreakpoints("res://eval.lua", {At(5)});
	f.Then([](LuaDebugger &debugger) {
		CHECK(debugger.Evaluate(0, "up + #s").value == "12");
		CHECK(debugger.Evaluate(0, "s .. '!'").value == "\"hi!\"");
		CHECK(debugger.Evaluate(0, "type(print)").value == "\"function\"");

		const LuaEvalResult failed = debugger.Evaluate(0, "nope.field");
		CHECK_FALSE(failed.success);
		CHECK(failed.value.find("nope") != std::string::npos);

		// Statements assign locals, upvalues and globals.
		CHECK(debugger.Evaluate(0, "s = 'changed'").success);
		CHECK(debugger.Evaluate(0, "s").value == "\"changed\"");
		CHECK(debugger.Evaluate(0, "created = up * 2").success);

		const i32 upvalues = ScopeRef(debugger, 0, "Upvalues");
		const LuaEvalResult up = debugger.SetVariable(upvalues, "up", "20");
		CHECK(up.success);
		CHECK(up.value == "20");

		const i32 locals = ScopeRef(debugger, 0, "Locals");
		const std::vector<LuaVariable> localVariables = debugger.Variables(locals);
		const LuaVariable *t = Find(localVariables, "t");
		REQUIRE(t != nullptr);
		const LuaEvalResult name = debugger.SetVariable(t->reference, "name", "s .. '?'");
		CHECK(name.success);
		CHECK(name.value == "\"changed?\"");

		CHECK_FALSE(debugger.SetVariable(locals, "missing", "1").success);
		return LuaResumeAction::Continue;
	});
	const auto result = f.Call("show");
	REQUIRE(result.valid());
	CHECK(result.get<int>(0) == 27);
	CHECK(result.get<std::string>(1) == "changed");
	CHECK(result.get<std::string>(2) == "changed?");
	CHECK(f.lua.get<int>("created") == 20);
}

TEST_CASE("Breakpoints hit inside coroutines made before and after they were set") {
	DebugFixture f;
	f.Load("co.lua", {
						 "function body()",		 // 1
						 "  local x = 1",		 // 2
						 "  coroutine.yield(x)", // 3
						 "  x = x + 1",			 // 4
						 "  return x",			 // 5
						 "end",					 // 6
						 "early = coroutine.create(body)",
					 });
	f.Load("drive.lua", {
							"function resumeEarly() return coroutine.resume(early) end",
							"function runWrapped()",
							"  local step = coroutine.wrap(body)",
							"  return step(), step()",
							"end",
						});
	CHECK(f.Call("resumeEarly").get<bool>()); // suspended at line 3
	f.debugger.SetBreakpoints("res://co.lua", {At(4)});

	f.Then([](LuaDebugger &debugger) {
		CHECK(debugger.Evaluate(0, "x").value == "1");
		return LuaResumeAction::Continue;
	});
	const auto resumed = f.Call("resumeEarly");
	CHECK(resumed.get<bool>(0));
	CHECK(resumed.get<int>(1) == 2);
	REQUIRE(f.stops.size() == 1);
	CHECK(f.stops[0].line == 4);

	const auto wrapped = f.Call("runWrapped");
	REQUIRE(wrapped.valid());
	CHECK(wrapped.get<int>(0) == 1);
	CHECK(wrapped.get<int>(1) == 2);
	CHECK(f.stops.size() == 2);
}

TEST_CASE("coroutine.wrap keeps its error behavior") {
	DebugFixture f;
	f.Load("wrap.lua", {
						   "function boom()",
						   "  local step = coroutine.wrap(function() error('inside') end)",
						   "  step()",
						   "end",
					   });
	const auto result = f.Call("boom");
	REQUIRE_FALSE(result.valid());
	CHECK(std::string(result.get<sol::error>().what()).find("inside") != std::string::npos);
}

TEST_CASE("A pause request stops an endless loop when the pause watch is on") {
	DebugFixture f;
	f.Load("spin.lua", {
						   "function spin()",				// 1
						   "  local n = 0",					// 2
						   "  while true do n = n + 1 end", // 3
						   "end",							// 4
					   });
	f.debugger.SetPauseWatch(true);
	f.Then([](LuaDebugger &debugger) {
		CHECK(debugger.StackTrace().at(0).name == "spin");
		return LuaResumeAction::Abort;
	});
	std::thread requester([&f] {
		std::this_thread::sleep_for(std::chrono::milliseconds(20));
		f.debugger.RequestPause();
	});
	const auto result = f.Call("spin");
	requester.join();

	REQUIRE_FALSE(result.valid());
	CHECK(std::string(result.get<sol::error>().what()).find(LuaDebugger::kAbortMessage) !=
		  std::string::npos);
	REQUIRE(f.stops.size() == 1);
	CHECK(f.stops[0].reason == LuaStopReason::Pause);
	CHECK(f.stops[0].line == 3);

	f.debugger.SetPauseWatch(false);
	CHECK(lua_gethook(f.lua.lua_state()) == nullptr);
}

TEST_CASE("Abort unwinds to the protected call without an error stop") {
	DebugFixture f;
	f.Load("abort.lua", {
							"function work()", // 1
							"  ran = 1",	   // 2
							"  ran = 2",	   // 3
							"end",			   // 4
						});
	f.debugger.SetBreakOnError(true);
	f.debugger.SetBreakpoints("res://abort.lua", {At(2)});
	f.Then(LuaResumeAction::Abort);
	const auto result = f.Call("work");
	REQUIRE_FALSE(result.valid());
	CHECK(std::string(result.get<sol::error>().what()).find(LuaDebugger::kAbortMessage) !=
		  std::string::npos);
	CHECK(f.stops.size() == 1);
	const sol::object ran = f.lua["ran"];
	CHECK(ran.get_type() == sol::type::lua_nil);
}

TEST_CASE("Destroying the debugger removes its hooks and coroutine wrappers") {
	sol::state lua;
	lua.open_libraries(sol::lib::base, sol::lib::coroutine);
	const sol::object create = lua["coroutine"]["create"];
	{
		LuaDebugger debugger(lua.lua_state());
		CHECK(lua["coroutine"]["create"] != create);
		debugger.SetBreakpoints("res://x.lua", {At(1)});
		CHECK(lua_gethook(lua.lua_state()) != nullptr);
	}
	CHECK(lua_gethook(lua.lua_state()) == nullptr);
	CHECK(lua["coroutine"]["create"] == create);
	CHECK(lua.safe_script("local co = coroutine.wrap(function() return 5 end) assert(co() == 5)")
			.valid());
}
