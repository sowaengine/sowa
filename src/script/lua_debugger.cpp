#include "script/lua_debugger.hpp"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <utility>

#include <fmt/format.h>

extern "C" {
#include <lauxlib.h>
#include <lua.h>
}

#include "core/log.hpp"

// Lua errors longjmp: raising operations run under lua_pcall so they cannot cross
// C++ destructors. Abort raises only after the C++ hook frames return.

namespace sowa {

namespace {

char kDebuggerKey;
char kThreadsKey;
char kOriginalCreateKey;
char kOriginalWrapKey;

// Instructions between count hooks while the pause watch is on.
constexpr int kPauseWatchCount = 1000;
// Children listed for one table or userdata.
constexpr std::size_t kMaxChildren = 1000;
// Characters of a string value shown before it is cut.
constexpr std::size_t kMaxStringDisplay = 200;

LuaDebugger *debuggerOf(lua_State *L) {
	lua_rawgetp(L, LUA_REGISTRYINDEX, &kDebuggerKey);
	auto *debugger = static_cast<LuaDebugger *>(lua_touserdata(L, -1));
	lua_pop(L, 1);
	return debugger;
}

// "res://scripts/player.lua" for a chunk named "@res://scripts/player.lua";
// empty for a chunk that did not come from a file.
std::string_view sourceKey(const char *source) {
	if (source == nullptr || source[0] != '@') {
		return {};
	}
	return std::string_view(source + 1);
}

i32 stackDepth(lua_State *L) {
	lua_Debug ar;
	i32 depth = 0;
	while (lua_getstack(L, depth, &ar) != 0) {
		++depth;
	}
	return depth;
}

// The global holding the function at index, or empty. Raw reads only.
std::string globalNameOf(lua_State *L, int index) {
	std::string name;
	lua_rawgeti(L, LUA_REGISTRYINDEX, LUA_RIDX_GLOBALS);
	lua_pushnil(L);
	while (lua_next(L, -2) != 0) {
		if (lua_type(L, -2) == LUA_TSTRING && lua_rawequal(L, -1, index) != 0) {
			name = lua_tostring(L, -2);
			lua_pop(L, 2);
			break;
		}
		lua_pop(L, 1);
	}
	lua_pop(L, 1);
	return name;
}

int toStringProtected(lua_State *L) {
	luaL_tolstring(L, 1, nullptr);
	return 1;
}

int setTableProtected(lua_State *L) {
	lua_settable(L, 1);
	return 0;
}

int getTableProtected(lua_State *L) {
	lua_gettable(L, 1);
	return 1;
}

bool isIdentifier(std::string_view text) {
	if (text.empty() || std::isdigit(static_cast<unsigned char>(text[0])) != 0) {
		return false;
	}
	return std::all_of(text.begin(), text.end(),
		[](char c) { return std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_'; });
}

std::string quote(std::string_view text) {
	std::string out = "\"";
	for (const char c : text) {
		switch (c) {
		case '"':
			out += "\\\"";
			break;
		case '\\':
			out += "\\\\";
			break;
		case '\n':
			out += "\\n";
			break;
		default:
			out.push_back(c);
		}
	}
	out.push_back('"');
	return out;
}

// Same as lcorolib.c's auxresume.
int auxResume(lua_State *L, lua_State *co, int narg) {
	if (!lua_checkstack(co, narg)) {
		lua_pushliteral(L, "too many arguments to resume");
		return -1;
	}
	lua_xmove(L, co, narg);
	int nres = 0;
	const int status = lua_resume(co, L, narg, &nres);
	if (status == LUA_OK || status == LUA_YIELD) {
		if (!lua_checkstack(L, nres + 1)) {
			lua_pop(co, nres);
			lua_pushliteral(L, "too many results to resume");
			return -1;
		}
		lua_xmove(co, L, nres);
		return nres;
	}
	lua_xmove(co, L, 1);
	return -1;
}

// Same as lcorolib.c's auxwrap, for threads made by the coroutine.wrap below.
int coroutineWrapResume(lua_State *L) {
	lua_State *co = lua_tothread(L, lua_upvalueindex(1));
	const int r = auxResume(L, co, lua_gettop(L));
	if (r < 0) {
		int stat = lua_status(co);
		if (stat != LUA_OK && stat != LUA_YIELD) {
			stat = lua_closethread(co, L);
			lua_xmove(co, L, 1);
		}
		if (stat != LUA_ERRMEM && lua_type(L, -1) == LUA_TSTRING) {
			luaL_where(L, 1);
			lua_insert(L, -2);
			lua_concat(L, 2);
		}
		return lua_error(L);
	}
	return r;
}

// coroutine.wrap(f): a thread from the debugger's coroutine.create (upvalue
// 1), so it is registered like any other.
int coroutineWrap(lua_State *L) {
	luaL_checktype(L, 1, LUA_TFUNCTION);
	lua_pushvalue(L, lua_upvalueindex(1));
	lua_pushvalue(L, 1);
	lua_call(L, 1, 1);
	lua_pushcclosure(L, coroutineWrapResume, 1);
	return 1;
}

} // namespace

// --- Lua entry points --------------------------------------------------------

void LuaDebuggerHook(lua_State *L, lua_Debug *ar) {
	LuaDebugger *debugger = debuggerOf(L);
	if (debugger == nullptr) {
		lua_sethook(L, nullptr, 0, 0);
		return;
	}
	if (debugger->onHook(L, ar)) {
		lua_pushstring(L, LuaDebugger::kAbortMessage);
		lua_error(L);
	}
}

// Message handler. Level 0 is this function, so a traceback from level 1
// starts at the function that raised the error. The message rule matches
// sol2's default_traceback_error_handler, keeping engine logs unchanged.
int LuaDebuggerErrorHandler(lua_State *L) {
	if (LuaDebugger *debugger = debuggerOf(L)) {
		debugger->onError(L);
	}
	const char *message = lua_type(L, 1) == LUA_TSTRING
							  ? lua_tostring(L, 1)
							  : "An unknown error has triggered the default error handler";
	luaL_traceback(L, L, message, 1);
	return 1;
}

// coroutine.create replacement. Upvalue 1 is the original.
int LuaDebuggerCoroutineCreate(lua_State *L) {
	lua_pushvalue(L, lua_upvalueindex(1));
	lua_insert(L, 1);
	lua_call(L, lua_gettop(L) - 1, 1);
	if (LuaDebugger *debugger = debuggerOf(L)) {
		if (lua_State *thread = lua_tothread(L, -1)) {
			debugger->registerThread(L, thread);
		}
	}
	return 1;
}

namespace {

// Resolves the frame an evaluation proxy reads: upvalues are the frame's
// level and the thread's depth when evaluation started; the stack has grown
// by the evaluation's own frames since.
bool proxyFrame(lua_State *L, lua_Debug *ar) {
	const auto level = static_cast<i32>(lua_tointeger(L, lua_upvalueindex(2)));
	const auto baseDepth = static_cast<i32>(lua_tointeger(L, lua_upvalueindex(3)));
	return lua_getstack(L, level + (stackDepth(L) - baseDepth), ar) != 0;
}

// Finds the last local named name (the innermost one in scope); 0 if none.
int findLocal(lua_State *L, lua_Debug *ar, const char *name) {
	int found = 0;
	for (int i = 1;; ++i) {
		const char *local = lua_getlocal(L, ar, i);
		if (local == nullptr) {
			break;
		}
		lua_pop(L, 1);
		if (std::strcmp(local, name) == 0) {
			found = i;
		}
	}
	return found;
}

// Pushes the frame's function and returns the index of upvalue name, or 0
// (function left pushed either way).
int findUpvalue(lua_State *L, lua_Debug *ar, const char *name) {
	lua_getinfo(L, "f", ar);
	for (int i = 1;; ++i) {
		const char *upvalue = lua_getupvalue(L, -1, i);
		if (upvalue == nullptr) {
			return 0;
		}
		lua_pop(L, 1);
		if (std::strcmp(upvalue, name) == 0) {
			return i;
		}
	}
}

} // namespace

// __index of an evaluation's _ENV: the frame's locals, then its upvalues,
// then globals.
int LuaDebuggerEnvIndex(lua_State *L) {
	const char *name = lua_tostring(L, 2);
	lua_Debug ar;
	if (name != nullptr && proxyFrame(L, &ar)) {
		if (const int local = findLocal(L, &ar, name); local != 0) {
			lua_getlocal(L, &ar, local);
			return 1;
		}
		if (const int upvalue = findUpvalue(L, &ar, name); upvalue != 0) {
			lua_getupvalue(L, -1, upvalue);
			return 1;
		}
		lua_pop(L, 1);
	}
	lua_rawgeti(L, LUA_REGISTRYINDEX, LUA_RIDX_GLOBALS);
	lua_pushvalue(L, 2);
	lua_gettable(L, -2);
	return 1;
}

// __newindex of an evaluation's _ENV: assigns the local, upvalue or global
// the same name would read.
int LuaDebuggerEnvNewIndex(lua_State *L) {
	const char *name = lua_tostring(L, 2);
	lua_Debug ar;
	if (name != nullptr && proxyFrame(L, &ar)) {
		if (const int local = findLocal(L, &ar, name); local != 0) {
			lua_pushvalue(L, 3);
			lua_setlocal(L, &ar, local);
			return 0;
		}
		if (const int upvalue = findUpvalue(L, &ar, name); upvalue != 0) {
			lua_pushvalue(L, 3);
			lua_setupvalue(L, -2, upvalue);
			return 0;
		}
		lua_pop(L, 1);
	}
	lua_rawgeti(L, LUA_REGISTRYINDEX, LUA_RIDX_GLOBALS);
	lua_pushvalue(L, 2);
	lua_pushvalue(L, 3);
	lua_settable(L, -3);
	return 0;
}

// --- Setup -------------------------------------------------------------------

LuaDebugger::LuaDebugger(lua_State *L) {
	lua_rawgeti(L, LUA_REGISTRYINDEX, LUA_RIDX_MAINTHREAD);
	_main = lua_tothread(L, -1);
	lua_pop(L, 1);

	lua_pushlightuserdata(_main, this);
	lua_rawsetp(_main, LUA_REGISTRYINDEX, &kDebuggerKey);

	// Weak-keyed set of every coroutine made since, so hooks can follow.
	lua_newtable(_main);
	lua_newtable(_main);
	lua_pushliteral(_main, "k");
	lua_setfield(_main, -2, "__mode");
	lua_setmetatable(_main, -2);
	lua_rawsetp(_main, LUA_REGISTRYINDEX, &kThreadsKey);

	lua_getglobal(_main, "coroutine");
	if (lua_istable(_main, -1)) {
		lua_getfield(_main, -1, "create");
		lua_getfield(_main, -2, "wrap");
		lua_rawsetp(_main, LUA_REGISTRYINDEX, &kOriginalWrapKey);
		lua_pushvalue(_main, -1);
		lua_rawsetp(_main, LUA_REGISTRYINDEX, &kOriginalCreateKey);
		lua_pushcclosure(_main, LuaDebuggerCoroutineCreate, 1);
		lua_pushvalue(_main, -1);
		lua_setfield(_main, -3, "create");
		lua_pushcclosure(_main, coroutineWrap, 1);
		lua_setfield(_main, -2, "wrap");
	}
	lua_pop(_main, 1);
}

LuaDebugger::~LuaDebugger() {
	_breakpoints.clear();
	_step = StepMode::None;
	_pauseWatch = false;
	_pauseRequested = false;
	refreshHooks();

	lua_getglobal(_main, "coroutine");
	if (lua_istable(_main, -1)) {
		lua_rawgetp(_main, LUA_REGISTRYINDEX, &kOriginalCreateKey);
		lua_setfield(_main, -2, "create");
		lua_rawgetp(_main, LUA_REGISTRYINDEX, &kOriginalWrapKey);
		lua_setfield(_main, -2, "wrap");
	}
	lua_pop(_main, 1);
	for (const char *key : {&kOriginalCreateKey, &kOriginalWrapKey, &kThreadsKey, &kDebuggerKey}) {
		lua_pushnil(_main);
		lua_rawsetp(_main, LUA_REGISTRYINDEX, key);
	}
	luaL_unref(_main, LUA_REGISTRYINDEX, _expanderRef);
}

void LuaDebugger::SetStopHandler(StopHandler handler) {
	_stopHandler = std::move(handler);
}

void LuaDebugger::SetOutputHandler(OutputHandler handler) {
	_outputHandler = std::move(handler);
}

void LuaDebugger::SetUserdataExpander(lua_State *L) {
	luaL_unref(L, LUA_REGISTRYINDEX, _expanderRef);
	_expanderRef = luaL_ref(L, LUA_REGISTRYINDEX);
}

void LuaDebugger::SetBreakpoints(std::string_view source, std::vector<LuaBreakpoint> breakpoints) {
	if (breakpoints.empty()) {
		if (const auto it = _breakpoints.find(source); it != _breakpoints.end()) {
			_breakpoints.erase(it);
		}
	} else {
		std::vector<BreakpointState> states;
		states.reserve(breakpoints.size());
		for (LuaBreakpoint &breakpoint : breakpoints) {
			states.push_back(BreakpointState{std::move(breakpoint)});
			parseHitCondition(states.back());
		}
		_breakpoints.insert_or_assign(std::string(source), std::move(states));
	}
	refreshHooks();
}

void LuaDebugger::ClearAllBreakpoints() {
	_breakpoints.clear();
	refreshHooks();
}

void LuaDebugger::SetBreakOnError(bool enabled) {
	_breakOnError = enabled;
}

void LuaDebugger::SetPauseWatch(bool enabled) {
	if (_pauseWatch == enabled) {
		return;
	}
	_pauseWatch = enabled;
	refreshHooks();
}

void LuaDebugger::RequestPause() {
	_pauseRequested = true;
}

void LuaDebugger::CancelPause() {
	_pauseRequested = false;
}

void LuaDebugger::PushErrorHandler(lua_State *L) {
	lua_pushcfunction(L, LuaDebuggerErrorHandler);
}

void LuaDebugger::registerThread(lua_State *L, lua_State *thread) {
	lua_rawgetp(L, LUA_REGISTRYINDEX, &kThreadsKey);
	lua_pushthread(thread);
	lua_xmove(thread, L, 1);
	lua_pushboolean(L, 1);
	lua_rawset(L, -3);
	lua_pop(L, 1);
	// A new thread copies its creator's hook, which may have line events on
	// for the creator's function; it has no function of its own yet.
	applyHook(thread, linesWanted());
}

// --- Hooks -------------------------------------------------------------------

lua_State *LuaDebugger::workingThread() const {
	return _stop.active ? _stop.thread : _main;
}

int LuaDebugger::baseMask() const {
	int mask = 0;
	if (!_breakpoints.empty() || _step != StepMode::None || _pauseRequested.load()) {
		mask |= LUA_MASKCALL | LUA_MASKRET;
	}
	if (_pauseWatch) {
		mask |= LUA_MASKCOUNT;
	}
	return mask;
}

bool LuaDebugger::linesWanted() const {
	return _step != StepMode::None || _pauseRequested.load();
}

bool LuaDebugger::sourceHasBreakpoints(const char *source) const {
	const std::string_view key = sourceKey(source);
	return !key.empty() && _breakpoints.find(key) != _breakpoints.end();
}

bool LuaDebugger::currentFunctionHasBreakpoints(lua_State *thread) {
	lua_Debug ar;
	return lua_getstack(thread, 0, &ar) != 0 && lua_getinfo(thread, "S", &ar) != 0 &&
		   sourceHasBreakpoints(ar.source);
}

void LuaDebugger::applyHook(lua_State *thread, bool lines) {
	int mask = baseMask();
	if (lines && mask != 0) {
		mask |= LUA_MASKLINE;
	}
	if (mask == lua_gethookmask(thread) && (mask == 0 || lua_gethook(thread) != nullptr)) {
		return;
	}
	if (mask == 0) {
		lua_sethook(thread, nullptr, 0, 0);
	} else {
		lua_sethook(thread, LuaDebuggerHook, mask, kPauseWatchCount);
	}
}

void LuaDebugger::refreshHooks(lua_State *running) {
	const bool lines = linesWanted();
	applyHook(_main, lines || currentFunctionHasBreakpoints(_main));
	lua_State *L = running != nullptr ? running : workingThread();
	lua_rawgetp(L, LUA_REGISTRYINDEX, &kThreadsKey);
	if (!lua_istable(L, -1)) {
		lua_pop(L, 1);
		return;
	}
	lua_pushnil(L);
	while (lua_next(L, -2) != 0) {
		lua_pop(L, 1);
		if (lua_State *thread = lua_tothread(L, -1)) {
			applyHook(thread, lines || currentFunctionHasBreakpoints(thread));
		}
	}
	lua_pop(L, 1);
}

bool LuaDebugger::onHook(lua_State *L, lua_Debug *ar) {
	if (_stop.active || _evaluating > 0) {
		return false;
	}
	switch (ar->event) {
	case LUA_HOOKCOUNT:
		if (_pauseRequested.load() && lua_getinfo(L, "Sl", ar) != 0 && ar->currentline >= 0) {
			return stop(L, LuaStopReason::Pause, 0, {}) == LuaResumeAction::Abort;
		}
		if (_pauseRequested.load()) {
			applyHook(L, true);
		}
		return false;
	case LUA_HOOKLINE:
		return onLine(L, ar);
	case LUA_HOOKCALL:
	case LUA_HOOKTAILCALL:
		if (linesWanted()) {
			applyHook(L, true);
			return false;
		}
		lua_getinfo(L, "S", ar);
		// A C function has no lines; the mask stays its caller's until a Lua
		// function it calls, or its return, sets it again.
		if (std::strcmp(ar->what, "C") != 0) {
			applyHook(L, sourceHasBreakpoints(ar->source));
		}
		return false;
	case LUA_HOOKRET: {
		// Level 0 is the returning function, level 1 its caller.
		if ((_step == StepMode::Over || _step == StepMode::Out) && L == _stepThread &&
			stackDepth(L) <= 1) {
			// Leaving the outermost frame: no line is left in this call to
			// step over or out to, so stepping ends as Continue.
			_step = StepMode::None;
			refreshHooks(L);
			return false;
		}
		if (linesWanted()) {
			return false;
		}
		lua_Debug caller;
		applyHook(L, lua_getstack(L, 1, &caller) != 0 && lua_getinfo(L, "S", &caller) != 0 &&
						 sourceHasBreakpoints(caller.source));
		return false;
	}
	default:
		return false;
	}
}

bool LuaDebugger::onLine(lua_State *L, lua_Debug *ar) {
	lua_getinfo(L, "Sl", ar);
	if (_pauseRequested.load()) {
		return stop(L, LuaStopReason::Pause, 0, {}) == LuaResumeAction::Abort;
	}
	if (_step != StepMode::None) {
		bool stopHere = _step == StepMode::In;
		if (!stopHere && L == _stepThread) {
			const i32 depth = stackDepth(L);
			stopHere = _step == StepMode::Over ? depth <= _stepDepth : depth < _stepDepth;
		}
		if (stopHere) {
			return stop(L, LuaStopReason::Step, 0, {}) == LuaResumeAction::Abort;
		}
	}

	const auto it = _breakpoints.find(sourceKey(ar->source));
	if (it == _breakpoints.end()) {
		return false;
	}
	const i32 line = ar->currentline;
	for (BreakpointState &breakpoint : it->second) {
		if (breakpoint.spec.line != line || !breakpointApplies(L, breakpoint)) {
			continue;
		}
		if (!breakpoint.spec.logMessage.empty()) {
			output(interpolate(L, breakpoint.spec.logMessage));
			continue;
		}
		return stop(L, LuaStopReason::Breakpoint, 0, {}) == LuaResumeAction::Abort;
	}
	return false;
}

void LuaDebugger::onError(lua_State *L) {
	if (!_breakOnError || _stop.active || _evaluating > 0 || !_stopHandler) {
		return;
	}
	std::string message;
	if (const char *text = lua_tostring(L, 1)) {
		message = text;
	} else {
		message = luaL_typename(L, 1);
	}
	if (message == kAbortMessage) {
		return;
	}
	// Level 0 is this handler; the error was raised by the first Lua frame
	// above it (an error call is a C frame in between).
	lua_Debug ar;
	i32 level = 1;
	while (lua_getstack(L, level, &ar) != 0) {
		lua_getinfo(L, "S", &ar);
		if (std::strcmp(ar.what, "C") != 0) {
			stop(L, LuaStopReason::Exception, level, std::move(message));
			return;
		}
		++level;
	}
}

LuaResumeAction LuaDebugger::stop(lua_State *L, LuaStopReason reason, i32 firstLevel,
	std::string message) {
	_pauseRequested = false;
	_step = StepMode::None;

	LuaStopEvent event;
	event.reason = reason;
	event.message = std::move(message);
	lua_Debug ar;
	if (lua_getstack(L, firstLevel, &ar) != 0 && lua_getinfo(L, "Sl", &ar) != 0) {
		event.source = std::string(sourceKey(ar.source));
		event.line = ar.currentline;
	}

	_stop = StopContext{true, L, stackDepth(L), firstLevel};
	LuaResumeAction action = _stopHandler ? _stopHandler(event) : LuaResumeAction::Continue;
	releaseRefs();
	const i32 depth = _stop.depth - firstLevel;
	_stop = StopContext{};

	if (reason == LuaStopReason::Exception &&
		(action == LuaResumeAction::Abort || action == LuaResumeAction::StepIn ||
			action == LuaResumeAction::StepOver || action == LuaResumeAction::StepOut)) {
		action = LuaResumeAction::Continue;
	}
	switch (action) {
	case LuaResumeAction::StepIn:
		_step = StepMode::In;
		break;
	case LuaResumeAction::StepOver:
		_step = StepMode::Over;
		break;
	case LuaResumeAction::StepOut:
		_step = StepMode::Out;
		break;
	default:
		break;
	}
	_stepThread = L;
	_stepDepth = depth;
	refreshHooks(L);
	return action;
}

// --- Breakpoint conditions -----------------------------------------------------

void LuaDebugger::parseHitCondition(BreakpointState &breakpoint) {
	std::string_view text = breakpoint.spec.hitCondition;
	const auto trim = [](std::string_view s) {
		while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front())) != 0) {
			s.remove_prefix(1);
		}
		while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back())) != 0) {
			s.remove_suffix(1);
		}
		return s;
	};
	text = trim(text);
	breakpoint.hitOp = HitOp::Always;
	if (text.empty()) {
		return;
	}
	static constexpr std::pair<std::string_view, HitOp> ops[] = {{">=", HitOp::GreaterEqual},
		{"<=", HitOp::LessEqual}, {"==", HitOp::Equal}, {">", HitOp::Greater}, {"<", HitOp::Less},
		{"%", HitOp::Modulo}};
	HitOp op = HitOp::GreaterEqual;
	for (const auto &[prefix, candidate] : ops) {
		if (text.starts_with(prefix)) {
			op = candidate;
			text = trim(text.substr(prefix.size()));
			break;
		}
	}
	i64 operand = 0;
	for (const char c : text) {
		if (std::isdigit(static_cast<unsigned char>(c)) == 0) {
			output(fmt::format("Ignoring invalid hit condition '{}' on line {}",
				breakpoint.spec.hitCondition, breakpoint.spec.line));
			return;
		}
		operand = operand * 10 + (c - '0');
	}
	if (op == HitOp::Modulo && operand == 0) {
		return;
	}
	breakpoint.hitOp = op;
	breakpoint.hitOperand = operand;
}

bool LuaDebugger::breakpointApplies(lua_State *L, BreakpointState &breakpoint) {
	if (!breakpoint.spec.condition.empty()) {
		const int top = lua_gettop(L);
		const bool evaluated = evaluateAt(L, 0, breakpoint.spec.condition);
		if (!evaluated) {
			// Stop so the broken condition is noticed rather than silently
			// never (or always) breaking.
			const char *error = lua_tostring(L, -1);
			output(fmt::format("Breakpoint condition '{}' failed: {}", breakpoint.spec.condition,
				error != nullptr ? error : "?"));
			lua_settop(L, top);
			return true;
		}
		const bool truthy = lua_toboolean(L, -1) != 0;
		lua_settop(L, top);
		if (!truthy) {
			return false;
		}
	}
	const i64 hits = ++breakpoint.hits;
	const i64 n = breakpoint.hitOperand;
	switch (breakpoint.hitOp) {
	case HitOp::Always:
		return true;
	case HitOp::GreaterEqual:
		return hits >= n;
	case HitOp::Greater:
		return hits > n;
	case HitOp::Equal:
		return hits == n;
	case HitOp::Less:
		return hits < n;
	case HitOp::LessEqual:
		return hits <= n;
	case HitOp::Modulo:
		return hits % n == 0;
	}
	return true;
}

std::string LuaDebugger::interpolate(lua_State *L, std::string_view message) {
	std::string out;
	std::size_t i = 0;
	while (i < message.size()) {
		const std::size_t open = message.find('{', i);
		if (open == std::string_view::npos) {
			break;
		}
		const std::size_t close = message.find('}', open);
		if (close == std::string_view::npos) {
			break;
		}
		out += message.substr(i, open - i);
		const int top = lua_gettop(L);
		if (evaluateAt(L, 0, message.substr(open + 1, close - open - 1))) {
			out += lua_type(L, -1) == LUA_TSTRING ? std::string(lua_tostring(L, -1))
												  : format(L, lua_gettop(L));
		} else {
			const char *error = lua_tostring(L, -1);
			out += fmt::format("<{}>", error != nullptr ? error : "error");
		}
		lua_settop(L, top);
		i = close + 1;
	}
	out += message.substr(i);
	return out;
}

void LuaDebugger::output(std::string_view text) {
	if (_outputHandler) {
		_outputHandler(text);
	} else {
		Log::Info("{}", text);
	}
}

// --- Evaluation ----------------------------------------------------------------

bool LuaDebugger::evaluateAt(lua_State *L, i32 level, std::string_view expression) {
	const std::string asExpression = "return " + std::string(expression);
	int status = luaL_loadbuffer(L, asExpression.data(), asExpression.size(), "=(debugger)");
	if (status != LUA_OK) {
		lua_pop(L, 1);
		status = luaL_loadbuffer(L, expression.data(), expression.size(), "=(debugger)");
		if (status != LUA_OK) {
			return false;
		}
	}

	// _ENV proxy reading and writing the frame's locals and upvalues.
	const i32 baseDepth = stackDepth(L);
	lua_newtable(L);
	lua_newtable(L);
	const auto pushAccessor = [&](lua_CFunction fn) {
		lua_pushlightuserdata(L, this);
		lua_pushinteger(L, level);
		lua_pushinteger(L, baseDepth);
		lua_pushcclosure(L, fn, 3);
	};
	pushAccessor(LuaDebuggerEnvIndex);
	lua_setfield(L, -2, "__index");
	pushAccessor(LuaDebuggerEnvNewIndex);
	lua_setfield(L, -2, "__newindex");
	lua_setmetatable(L, -2);
	lua_setupvalue(L, -2, 1);

	++_evaluating;
	status = lua_pcall(L, 0, 1, 0);
	--_evaluating;
	return status == LUA_OK;
}

// --- Inspection ------------------------------------------------------------------

i32 LuaDebugger::levelOf(i32 frameId) const {
	return _stop.firstLevel + frameId;
}

i32 LuaDebugger::addRef(Ref ref) {
	_refs.push_back(ref);
	return static_cast<i32>(_refs.size());
}

void LuaDebugger::releaseRefs() {
	for (const Ref &ref : _refs) {
		luaL_unref(_stop.thread, LUA_REGISTRYINDEX, ref.value);
		luaL_unref(_stop.thread, LUA_REGISTRYINDEX, ref.children);
	}
	_refs.clear();
}

std::vector<LuaStackFrame> LuaDebugger::StackTrace() {
	std::vector<LuaStackFrame> frames;
	if (!_stop.active) {
		return frames;
	}
	lua_State *L = _stop.thread;
	lua_Debug ar;
	for (i32 level = _stop.firstLevel; lua_getstack(L, level, &ar) != 0; ++level) {
		lua_getinfo(L, "Snl", &ar);
		LuaStackFrame frame;
		frame.id = level - _stop.firstLevel;
		frame.source = std::string(sourceKey(ar.source));
		frame.line = ar.currentline;
		if (ar.name != nullptr) {
			frame.name = ar.name;
		} else if (std::strcmp(ar.what, "main") == 0) {
			frame.name = "main chunk";
		} else {
			// Called from C (the engine), so Lua has no name for it.
			lua_getinfo(L, "f", &ar);
			frame.name = globalNameOf(L, lua_gettop(L));
			lua_pop(L, 1);
			if (frame.name.empty()) {
				frame.name = std::strcmp(ar.what, "C") == 0
								 ? "[C]"
								 : fmt::format("function <line {}>", ar.linedefined);
			}
		}
		frames.push_back(std::move(frame));
	}
	return frames;
}

std::vector<LuaScope> LuaDebugger::Scopes(i32 frameId) {
	std::vector<LuaScope> scopes;
	lua_Debug ar;
	if (!_stop.active || lua_getstack(_stop.thread, levelOf(frameId), &ar) == 0) {
		return scopes;
	}
	const i32 level = levelOf(frameId);
	scopes.push_back({"Locals", addRef(Ref{RefKind::Locals, level})});
	scopes.push_back({"Upvalues", addRef(Ref{RefKind::Upvalues, level})});
	scopes.push_back({"Globals", addRef(Ref{RefKind::Globals, level})});
	return scopes;
}

std::string LuaDebugger::format(lua_State *L, int index) {
	index = lua_absindex(L, index);
	switch (lua_type(L, index)) {
	case LUA_TNIL:
		return "nil";
	case LUA_TBOOLEAN:
		return lua_toboolean(L, index) != 0 ? "true" : "false";
	case LUA_TNUMBER:
		if (lua_isinteger(L, index) != 0) {
			return fmt::format("{}", static_cast<long long>(lua_tointeger(L, index)));
		}
		return fmt::format("{:.14g}", lua_tonumber(L, index));
	case LUA_TSTRING: {
		std::size_t length = 0;
		const char *text = lua_tolstring(L, index, &length);
		std::string_view view(text, length);
		if (view.size() > kMaxStringDisplay) {
			return quote(view.substr(0, kMaxStringDisplay)) + "...";
		}
		return quote(view);
	}
	case LUA_TFUNCTION: {
		lua_Debug ar;
		lua_pushvalue(L, index);
		lua_getinfo(L, ">S", &ar);
		if (std::strcmp(ar.what, "C") == 0) {
			return "function [C]";
		}
		const std::string_view source = sourceKey(ar.source);
		return fmt::format("function {}:{}", source.empty() ? ar.short_src : source,
			ar.linedefined);
	}
	case LUA_TTABLE:
		if (luaL_getmetafield(L, index, "__tostring") == LUA_TNIL &&
			luaL_getmetafield(L, index, "__name") == LUA_TNIL) {
			std::size_t count = 0;
			lua_pushnil(L);
			while (lua_next(L, index) != 0) {
				lua_pop(L, 1);
				if (++count > kMaxChildren) {
					lua_pop(L, 1);
					return fmt::format("table {{{}+}}", kMaxChildren);
				}
			}
			return fmt::format("table {{{}}}", count);
		}
		lua_pop(L, 1);
		[[fallthrough]];
	default: {
		lua_pushcfunction(L, toStringProtected);
		lua_pushvalue(L, index);
		std::string text = luaL_typename(L, index);
		if (lua_pcall(L, 1, 1, 0) == LUA_OK) {
			text = lua_tostring(L, -1);
		}
		lua_pop(L, 1);
		return text;
	}
	}
}

LuaVariable LuaDebugger::describe(lua_State *L, int index, std::string name, i32 level) {
	index = lua_absindex(L, index);
	LuaVariable variable;
	variable.name = std::move(name);
	variable.type = luaL_typename(L, index);
	variable.value = format(L, index);
	const int type = lua_type(L, index);
	if (type == LUA_TTABLE || (type == LUA_TUSERDATA && _expanderRef >= 0)) {
		lua_pushvalue(L, index);
		variable.reference = addRef(Ref{RefKind::Value, level, luaL_ref(L, LUA_REGISTRYINDEX)});
	}
	return variable;
}

namespace {

std::string keyName(lua_State *L, int index) {
	switch (lua_type(L, index)) {
	case LUA_TSTRING: {
		const char *key = lua_tostring(L, index);
		return isIdentifier(key) ? std::string(key) : "[" + quote(key) + "]";
	}
	case LUA_TNUMBER:
		if (lua_isinteger(L, index) != 0) {
			return fmt::format("[{}]", static_cast<long long>(lua_tointeger(L, index)));
		}
		return fmt::format("[{:.14g}]", lua_tonumber(L, index));
	case LUA_TBOOLEAN:
		return lua_toboolean(L, index) != 0 ? "[true]" : "[false]";
	default:
		return fmt::format("[{}]", luaL_typename(L, index));
	}
}

// Numeric keys first in order, then names alphabetically.
struct ChildEntry {
	bool numeric = false;
	f64 number = 0.0;
	LuaVariable variable;
};

// Pushes the key a child named name (as keyName made it) refers to.
void pushKey(lua_State *L, std::string_view name) {
	if (name.size() >= 2 && name.front() == '[' && name.back() == ']') {
		std::string_view inner = name.substr(1, name.size() - 2);
		if (inner.size() >= 2 && inner.front() == '"' && inner.back() == '"') {
			std::string key;
			inner = inner.substr(1, inner.size() - 2);
			for (std::size_t i = 0; i < inner.size(); ++i) {
				if (inner[i] == '\\' && i + 1 < inner.size()) {
					++i;
					key.push_back(inner[i] == 'n' ? '\n' : inner[i]);
				} else {
					key.push_back(inner[i]);
				}
			}
			lua_pushlstring(L, key.data(), key.size());
			return;
		}
		if (inner == "true" || inner == "false") {
			lua_pushboolean(L, inner == "true" ? 1 : 0);
			return;
		}
		const std::string number(inner);
		if (lua_stringtonumber(L, number.c_str()) != 0) {
			return;
		}
	}
	lua_pushlstring(L, name.data(), name.size());
}

} // namespace

std::vector<LuaVariable> LuaDebugger::Variables(i32 reference) {
	std::vector<LuaVariable> variables;
	if (!_stop.active || reference <= 0 || reference > static_cast<i32>(_refs.size())) {
		return variables;
	}
	lua_State *L = _stop.thread;
	const Ref ref = _refs[reference - 1];
	const int top = lua_gettop(L);
	lua_Debug ar;

	switch (ref.kind) {
	case RefKind::Locals:
		if (lua_getstack(L, ref.level, &ar) != 0) {
			for (int i = 1;; ++i) {
				const char *name = lua_getlocal(L, &ar, i);
				if (name == nullptr) {
					break;
				}
				if (name[0] != '(') {
					variables.push_back(describe(L, -1, name, ref.level));
				}
				lua_pop(L, 1);
			}
			for (int i = 1;; ++i) {
				if (lua_getlocal(L, &ar, -i) == nullptr) {
					break;
				}
				variables.push_back(describe(L, -1, fmt::format("(vararg {})", i), ref.level));
				lua_pop(L, 1);
			}
		}
		break;
	case RefKind::Upvalues:
		if (lua_getstack(L, ref.level, &ar) != 0) {
			lua_getinfo(L, "f", &ar);
			const int function = lua_gettop(L);
			for (int i = 1;; ++i) {
				const char *name = lua_getupvalue(L, function, i);
				if (name == nullptr) {
					break;
				}
				if (std::strcmp(name, "_ENV") != 0) {
					variables.push_back(describe(L, -1, name[0] != '\0' ? name : "?", ref.level));
				}
				lua_pop(L, 1);
			}
		}
		break;
	case RefKind::Globals:
	case RefKind::Value: {
		if (ref.kind == RefKind::Globals) {
			lua_rawgeti(L, LUA_REGISTRYINDEX, LUA_RIDX_GLOBALS);
		} else if (lua_rawgeti(L, LUA_REGISTRYINDEX, ref.value) == LUA_TUSERDATA) {
			// Userdata children come from the expander, once per stop.
			if (ref.children == LUA_NOREF) {
				lua_rawgeti(L, LUA_REGISTRYINDEX, _expanderRef);
				lua_pushvalue(L, -2);
				if (lua_pcall(L, 1, 1, 0) == LUA_OK && lua_istable(L, -1)) {
					_refs[reference - 1].children = luaL_ref(L, LUA_REGISTRYINDEX);
				} else {
					lua_pop(L, 1);
				}
			}
			if (_refs[reference - 1].children == LUA_NOREF) {
				break;
			}
			lua_rawgeti(L, LUA_REGISTRYINDEX, _refs[reference - 1].children);
		}
		const int table = lua_gettop(L);
		if (!lua_istable(L, table)) {
			break;
		}
		std::vector<ChildEntry> entries;
		lua_pushnil(L);
		while (lua_next(L, table) != 0 && entries.size() < kMaxChildren) {
			ChildEntry entry;
			entry.numeric = lua_type(L, -2) == LUA_TNUMBER;
			entry.number = entry.numeric ? lua_tonumber(L, -2) : 0.0;
			entry.variable = describe(L, -1, keyName(L, -2), ref.level);
			entries.push_back(std::move(entry));
			lua_pop(L, 1);
		}
		std::sort(entries.begin(), entries.end(), [](const ChildEntry &a, const ChildEntry &b) {
			if (a.numeric != b.numeric) {
				return a.numeric;
			}
			return a.numeric ? a.number < b.number : a.variable.name < b.variable.name;
		});
		for (ChildEntry &entry : entries) {
			variables.push_back(std::move(entry.variable));
		}
		break;
	}
	}
	lua_settop(L, top);
	return variables;
}

LuaEvalResult LuaDebugger::Evaluate(i32 frameId, std::string_view expression) {
	LuaEvalResult result;
	if (!_stop.active) {
		result.value = "not stopped";
		return result;
	}
	lua_State *L = _stop.thread;
	const int top = lua_gettop(L);
	const i32 level = levelOf(frameId);
	result.success = evaluateAt(L, level, expression);
	if (result.success) {
		const LuaVariable variable = describe(L, -1, {}, level);
		result.value = variable.value;
		result.type = variable.type;
		result.reference = variable.reference;
	} else {
		const char *error = lua_tostring(L, -1);
		result.value = error != nullptr ? error : "error";
	}
	lua_settop(L, top);
	return result;
}

LuaEvalResult LuaDebugger::SetVariable(i32 reference, std::string_view name,
	std::string_view value) {
	LuaEvalResult result;
	if (!_stop.active || reference <= 0 || reference > static_cast<i32>(_refs.size())) {
		result.value = "not stopped";
		return result;
	}
	lua_State *L = _stop.thread;
	const Ref ref = _refs[reference - 1];
	const int top = lua_gettop(L);
	if (!evaluateAt(L, ref.level, value)) {
		const char *error = lua_tostring(L, -1);
		result.value = error != nullptr ? error : "error";
		lua_settop(L, top);
		return result;
	}
	const int newValue = lua_gettop(L);
	const std::string key(name);
	lua_Debug ar;
	bool assigned = false;

	switch (ref.kind) {
	case RefKind::Locals:
		if (lua_getstack(L, ref.level, &ar) != 0) {
			int local = 0;
			if (key.starts_with("(vararg ")) {
				local = -std::atoi(key.c_str() + 8);
			} else {
				local = findLocal(L, &ar, key.c_str());
			}
			if (local != 0) {
				lua_pushvalue(L, newValue);
				assigned = lua_setlocal(L, &ar, local) != nullptr;
				if (!assigned) {
					lua_pop(L, 1);
				}
			}
		}
		break;
	case RefKind::Upvalues:
		if (lua_getstack(L, ref.level, &ar) != 0) {
			if (const int upvalue = findUpvalue(L, &ar, key.c_str()); upvalue != 0) {
				lua_pushvalue(L, newValue);
				assigned = lua_setupvalue(L, -2, upvalue) != nullptr;
			}
		}
		break;
	case RefKind::Globals:
	case RefKind::Value:
		lua_pushcfunction(L, setTableProtected);
		if (ref.kind == RefKind::Globals) {
			lua_rawgeti(L, LUA_REGISTRYINDEX, LUA_RIDX_GLOBALS);
		} else {
			lua_rawgeti(L, LUA_REGISTRYINDEX, ref.value);
		}
		pushKey(L, name);
		lua_pushvalue(L, newValue);
		if (lua_pcall(L, 3, 0, 0) == LUA_OK) {
			assigned = true;
		} else {
			const char *error = lua_tostring(L, -1);
			result.value = error != nullptr ? error : "error";
			lua_settop(L, top);
			return result;
		}
		break;
	}

	if (!assigned) {
		result.value = fmt::format("no variable named '{}'", key);
		lua_settop(L, top);
		return result;
	}
	// Read back through the container so the shown value is what it holds
	// now (a __newindex may have converted it).
	if (ref.kind == RefKind::Value) {
		lua_pushcfunction(L, getTableProtected);
		lua_rawgeti(L, LUA_REGISTRYINDEX, ref.value);
		pushKey(L, name);
		if (lua_pcall(L, 2, 1, 0) != LUA_OK) {
			lua_pop(L, 1);
			lua_pushvalue(L, newValue);
		}
	} else {
		lua_pushvalue(L, newValue);
	}
	const LuaVariable variable = describe(L, -1, key, ref.level);
	result.success = true;
	result.value = variable.value;
	result.type = variable.type;
	result.reference = variable.reference;
	lua_settop(L, top);
	return result;
}

} // namespace sowa
