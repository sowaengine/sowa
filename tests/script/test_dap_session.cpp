#include <doctest/doctest.h>

#include <deque>
#include <functional>
#include <initializer_list>
#include <optional>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>
#include <sol/sol.hpp>

#include "script/dap_session.hpp"
#include "script/lua_debugger.hpp"

using namespace sowa;
using Json = nlohmann::json;

namespace {

struct LuaWithLibraries {
	sol::state lua;

	LuaWithLibraries() {
		lua.open_libraries(sol::lib::base, sol::lib::coroutine, sol::lib::string);
	}
};

// A client talking to a DapSession over a bare debugger, the way Application
// connects them: stops are forwarded to the session, and the "suspension"
// runs scripted requests until one of them resumes.
struct SessionFixture : LuaWithLibraries {
	LuaDebugger debugger{lua.lua_state()};
	std::vector<Json> sent;
	std::optional<LuaResumeAction> resumed;
	// Every resume, in order.
	std::vector<LuaResumeAction> resumes;
	std::deque<std::function<void()>> duringStop;
	DapSession session{debugger, [this](std::string json) { sent.push_back(Json::parse(json)); },
		DapSession::Hooks{[this](LuaResumeAction action) {
							  resumed = action;
							  resumes.push_back(action);
						  },
			{}}};
	int seq = 0;

	SessionFixture() {
		lua_State *L = lua.lua_state();
		debugger.PushErrorHandler(L);
		lua_setglobal(L, sol::detail::default_handler_name());
		debugger.SetStopHandler([this](const LuaStopEvent &event) {
			session.OnStopped(event);
			resumed.reset();
			if (!duringStop.empty()) {
				auto requests = std::move(duringStop.front());
				duringStop.pop_front();
				requests();
			}
			return resumed.value_or(LuaResumeAction::Continue);
		});
	}

	Json Request(const std::string &command, Json arguments = Json::object()) {
		const int id = ++seq;
		session.Handle(Json{{"seq", id}, {"type", "request"}, {"command", command},
			{"arguments", std::move(arguments)}}
				.dump());
		for (auto it = sent.rbegin(); it != sent.rend(); ++it) {
			if (it->value("type", "") == "response" && it->value("request_seq", 0) == id) {
				return *it;
			}
		}
		FAIL("no response to " << command);
		return {};
	}

	std::vector<Json> Events(const std::string &name) const {
		std::vector<Json> events;
		for (const Json &message : sent) {
			if (message.value("type", "") == "event" && message.value("event", "") == name) {
				events.push_back(message);
			}
		}
		return events;
	}

	void Attach() {
		REQUIRE(Request("initialize")["success"] == true);
		REQUIRE(Request("attach", {{"localRoot", "/proj"}})["success"] == true);
	}

	void Load(const std::string &resource, std::initializer_list<const char *> lines) {
		std::string code;
		for (const char *line : lines) {
			code += line;
			code += '\n';
		}
		REQUIRE(lua.safe_script(code, sol::script_pass_on_error, "@" + resource).valid());
	}

	sol::protected_function_result Call(const char *function, auto &&...args) {
		sol::protected_function fn = lua[function];
		return fn(std::forward<decltype(args)>(args)...);
	}

	Json SetBreakpoints(const std::string &path, std::initializer_list<Json> breakpoints) {
		return Request("setBreakpoints",
			{{"source", {{"path", path}}}, {"breakpoints", Json(std::vector<Json>(breakpoints))}});
	}
};

} // namespace

TEST_CASE("initialize answers capabilities, then sends initialized") {
	SessionFixture f;
	const Json response = f.Request("initialize");
	CHECK(response["success"] == true);
	CHECK(response["body"]["supportsConfigurationDoneRequest"] == true);
	CHECK(response["body"]["supportsConditionalBreakpoints"] == true);
	CHECK(response["body"]["supportsHitConditionalBreakpoints"] == true);
	CHECK(response["body"]["supportsLogPoints"] == true);
	CHECK(response["body"]["supportsSetVariable"] == true);
	CHECK(response["body"]["exceptionBreakpointFilters"][0]["filter"] == "uncaught");
	REQUIRE(f.sent.size() == 2);
	CHECK(f.sent[1]["event"] == "initialized");
	CHECK(f.sent[1]["seq"].get<int>() > f.sent[0]["seq"].get<int>());
}

TEST_CASE("Client paths map to res:// through the attach root") {
	SessionFixture f;
	f.session.SetProjectRoot("/engine/project");
	CHECK(f.session.ToResource("/engine/project/scripts/a.lua") == "res://scripts/a.lua");
	CHECK(f.session.ToPath("res://scripts/a.lua") ==
		  std::filesystem::path("/engine/project/scripts/a.lua").string());

	f.Attach(); // localRoot "/proj" takes precedence while attached
	CHECK(f.session.ToResource("/proj/scripts/../scripts/a.lua") == "res://scripts/a.lua");
	CHECK_FALSE(f.session.ToResource("/project/a.lua").has_value());
	CHECK_FALSE(f.session.ToResource("/elsewhere/a.lua").has_value());
	CHECK(f.session.ToPath("res://a.lua") == std::filesystem::path("/proj/a.lua").string());
	CHECK(f.session.ToPath("tmp").empty());

	f.Request("disconnect");
	CHECK(f.session.ToResource("/engine/project/a.lua") == "res://a.lua");
}

TEST_CASE("A breakpoint set by the client stops with a stopped event and full inspection") {
	SessionFixture f;
	f.Attach();
	f.Load("res://scripts/a.lua", {
									  "function tick(n)",  // 1
									  "  local x = n * 2", // 2
									  "  total = x + 1",   // 3
									  "  return total",	   // 4
									  "end",			   // 5
								  });
	const Json set = f.SetBreakpoints("/proj/scripts/a.lua", {Json{{"line", 3}}});
	CHECK(set["success"] == true);
	CHECK(set["body"]["breakpoints"][0]["verified"] == true);
	CHECK(f.Request("configurationDone")["success"] == true);
	CHECK(f.session.IsConfigured());

	f.duringStop.push_back([&] {
		const auto stopped = f.Events("stopped");
		REQUIRE(stopped.size() == 1);
		CHECK(stopped[0]["body"]["reason"] == "breakpoint");
		CHECK(stopped[0]["body"]["threadId"] == 1);

		CHECK(f.Request("threads")["body"]["threads"][0]["id"] == 1);

		const Json trace = f.Request("stackTrace", {{"threadId", 1}});
		REQUIRE(trace["success"] == true);
		const Json &top = trace["body"]["stackFrames"][0];
		CHECK(top["name"] == "tick");
		CHECK(top["line"] == 3);
		CHECK(top["source"]["path"] == std::filesystem::path("/proj/scripts/a.lua").string());
		CHECK(top["source"]["name"] == "a.lua");

		const Json scopes = f.Request("scopes", {{"frameId", top["id"]}});
		REQUIRE(scopes["success"] == true);
		CHECK(scopes["body"]["scopes"][0]["name"] == "Locals");
		CHECK(scopes["body"]["scopes"][2]["expensive"] == true);
		const int locals = scopes["body"]["scopes"][0]["variablesReference"];

		const Json variables = f.Request("variables", {{"variablesReference", locals}});
		bool sawX = false;
		for (const Json &variable : variables["body"]["variables"]) {
			if (variable["name"] == "x") {
				sawX = true;
				CHECK(variable["value"] == "10");
				CHECK(variable["type"] == "number");
			}
		}
		CHECK(sawX);

		const Json repl = f.Request("evaluate", {{"expression", "x + n"}, {"frameId", 0}});
		CHECK(repl["body"]["result"] == "15");
		CHECK(f.Request("evaluate", {{"expression", "x"}, {"frameId", 0},
										{"context", "hover"}})["body"]["result"] == "10");
		// Hovers never run code.
		CHECK(f.Request("evaluate", {{"expression", "tick(1)"}, {"frameId", 0},
										{"context", "hover"}})["success"] == false);
		const Json failed = f.Request("evaluate", {{"expression", "nope.x"}, {"frameId", 0}});
		CHECK(failed["success"] == false);
		CHECK(failed["message"].get<std::string>().find("nope") != std::string::npos);

		const Json set = f.Request("setVariable",
			{{"variablesReference", locals}, {"name", "x"}, {"value", "100"}});
		CHECK(set["success"] == true);
		CHECK(set["body"]["value"] == "100");

		const Json continued = f.Request("continue", {{"threadId", 1}});
		CHECK(continued["success"] == true);
		CHECK(continued["body"]["allThreadsContinued"] == true);
	});
	CHECK(f.Call("tick", 5).valid());
	CHECK(f.resumed == LuaResumeAction::Continue);
	CHECK(f.lua.get<int>("total") == 101);
}

TEST_CASE("Breakpoints outside the project are not verified and never stop") {
	SessionFixture f;
	f.Attach();
	const Json set = f.SetBreakpoints("/other/a.lua", {Json{{"line", 1}}});
	CHECK(set["body"]["breakpoints"][0]["verified"] == false);
	CHECK(set["body"]["breakpoints"][0].contains("message"));
	CHECK(lua_gethook(f.lua.lua_state()) == nullptr);
}

TEST_CASE("Step requests resume with the matching action") {
	SessionFixture f;
	f.Attach();
	f.Load("res://s.lua", {"function f()", "  local a = 1", "  return a", "end"});
	f.SetBreakpoints("/proj/s.lua", {Json{{"line", 2}}});
	for (const auto &[command, action] :
		{std::pair{"next", LuaResumeAction::StepOver}, std::pair{"stepIn", LuaResumeAction::StepIn},
			std::pair{"stepOut", LuaResumeAction::StepOut}}) {
		f.duringStop.push_back([&f, command = command] {
			CHECK(f.Request(command, {{"threadId", 1}})["success"] == true);
		});
		f.resumes.clear();
		f.Call("f");
		REQUIRE_FALSE(f.resumes.empty());
		CHECK(f.resumes.front() == action);
		f.duringStop.clear();
	}
}

TEST_CASE("Inspection and resume requests fail while Lua runs") {
	SessionFixture f;
	f.Attach();
	for (const char *command : {"stackTrace", "scopes", "variables", "evaluate", "setVariable",
			 "continue", "next", "stepIn", "stepOut"}) {
		const Json response = f.Request(command);
		CHECK_MESSAGE(response["success"] == false, command);
		CHECK(response["message"] == "Lua is not stopped");
	}
	CHECK_FALSE(f.resumed.has_value());
}

TEST_CASE("The uncaught filter turns break-on-error on and off") {
	SessionFixture f;
	f.Attach();
	f.Load("res://e.lua", {"function bad()", "  error('broken')", "end"});
	CHECK(f.Request("setExceptionBreakpoints",
			  {{"filters", Json::array({"uncaught"})}})["success"] == true);
	f.Call("bad");
	REQUIRE(f.Events("stopped").size() == 1);
	CHECK(f.Events("stopped")[0]["body"]["reason"] == "exception");
	CHECK(f.Events("stopped")[0]["body"]["text"].get<std::string>().find("broken") !=
		  std::string::npos);

	f.Request("setExceptionBreakpoints", {{"filters", Json::array()}});
	f.Call("bad");
	CHECK(f.Events("stopped").size() == 1);
}

TEST_CASE("pause stops a running loop") {
	SessionFixture f;
	f.Attach();
	f.Load("res://spin.lua", {"function spin()", "  while true do end", "end"});
	f.debugger.SetPauseWatch(true);
	f.duringStop.push_back([&] { f.resumed = LuaResumeAction::Abort; });
	// Handled on the main thread between frames; the count hook of the pause
	// watch then stops the loop that starts afterwards.
	CHECK(f.Request("pause", {{"threadId", 1}})["success"] == true);
	CHECK_FALSE(f.Call("spin").valid());
	REQUIRE(f.Events("stopped").size() == 1);
	CHECK(f.Events("stopped")[0]["body"]["reason"] == "pause");
}

TEST_CASE("disconnect removes breakpoints and resumes a stop") {
	SessionFixture f;
	f.Attach();
	f.Load("res://d.lua", {"function f()", "  hit = (hit or 0) + 1", "end"});
	f.SetBreakpoints("/proj/d.lua", {Json{{"line", 2}}});
	f.Request("setExceptionBreakpoints", {{"filters", Json::array({"uncaught"})}});

	SUBCASE("disconnect request") {
		f.duringStop.push_back([&] { CHECK(f.Request("disconnect")["success"] == true); });
	}
	SUBCASE("connection lost") {
		f.duringStop.push_back([&] { f.session.OnConnectionClosed(); });
	}
	f.Call("f");
	CHECK(f.resumed == LuaResumeAction::Continue);
	CHECK_FALSE(f.session.IsAttached());
	CHECK_FALSE(f.session.IsConfigured());
	CHECK(lua_gethook(f.lua.lua_state()) == nullptr);

	f.Call("f");
	CHECK(f.Events("stopped").size() == 1);
	CHECK(f.lua.get<int>("hit") == 2);
}

TEST_CASE("Engine events reach only an attached client") {
	SessionFixture f;
	f.session.OnOutput("before");
	f.session.OnContinued();
	CHECK(f.sent.empty());

	f.Attach();
	f.sent.clear();
	f.session.OnOutput("logpoint text");
	f.session.OnContinued();
	f.session.OnTerminated();
	REQUIRE(f.sent.size() == 3);
	CHECK(f.sent[0]["event"] == "output");
	CHECK(f.sent[0]["body"]["output"] == "logpoint text\n");
	CHECK(f.sent[1]["event"] == "continued");
	CHECK(f.sent[2]["event"] == "terminated");
}

TEST_CASE("Bad requests are answered with an error, never thrown") {
	SessionFixture f;
	f.Attach();
	const Json wrongType = f.SetBreakpoints("/proj/x.lua", {Json{{"line", "three"}}});
	CHECK(wrongType["success"] == false);
	CHECK(f.Request("launch")["success"] == false);
	CHECK(f.Request("restartFrame")["success"] == false);
	const std::size_t before = f.sent.size();
	f.session.Handle("not json");
	f.session.Handle(R"({"type":"event","event":"x"})");
	CHECK(f.sent.size() == before);
}
