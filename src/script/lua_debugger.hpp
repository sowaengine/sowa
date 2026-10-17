#pragma once

#include <atomic>
#include <functional>
#include <map>
#include <string>
#include <string_view>
#include <vector>

#include "core/types.hpp"

struct lua_State;
struct lua_Debug;

namespace sowa {

// Source-line breakpoint. Source is the chunk name without @, such as res://scripts/player.lua.
struct LuaBreakpoint {
	i32 line = 0;
	// Lua expression; the breakpoint applies only while it is truthy. An
	// expression that fails to evaluate stops, so the mistake is visible.
	std::string condition;
	// Applies once the hit count (hits with a passing condition) satisfies
	// it: "N" or ">= N", "> N", "== N", "< N", "<= N", or "% N" for every
	// Nth hit. Empty applies on every hit.
	std::string hitCondition;
	// Non-empty makes this a logpoint: the message is written to the output
	// handler instead of stopping. "{expr}" is replaced by the expression's
	// value in the stopped frame.
	std::string logMessage;
};

enum class LuaStopReason {
	Breakpoint,
	Step,
	Pause,
	Exception,
};

struct LuaStopEvent {
	LuaStopReason reason = LuaStopReason::Breakpoint;
	std::string source;
	i32 line = 0;
	// The error value, for an Exception stop.
	std::string message;
};

enum class LuaResumeAction {
	Continue,
	StepIn,
	StepOver,
	StepOut,
	// Unwinds the running Lua call with kAbortMessage, back to whichever
	// protected call started it. Ignored for an Exception stop, which is
	// already unwinding.
	Abort,
};

struct LuaStackFrame {
	i32 id = 0;
	std::string name;
	// Empty for a C function.
	std::string source;
	i32 line = 0;
};

struct LuaScope {
	std::string name;
	i32 reference = 0;
};

struct LuaVariable {
	std::string name;
	std::string value;
	std::string type;
	// Non-zero when the value has children (Variables(reference)).
	i32 reference = 0;
};

struct LuaEvalResult {
	bool success = false;
	// The value, or the error message when !success.
	std::string value;
	std::string type;
	i32 reference = 0;
};

// Synchronous stop handler for breakpoints, stepping, and inspection. Inspection
// references live only during the handler. Hooks follow debugger state on all coroutines.
class LuaDebugger {
  public:
	static constexpr const char *kAbortMessage = "execution stopped by the debugger";

	using StopHandler = std::function<LuaResumeAction(const LuaStopEvent &)>;
	using OutputHandler = std::function<void(std::string_view)>;

	// L may be any thread of the state; the debugger works on all of them.
	// Construct after the coroutine library is open, so coroutines created
	// from Lua are known.
	explicit LuaDebugger(lua_State *L);
	~LuaDebugger();

	LuaDebugger(const LuaDebugger &) = delete;
	LuaDebugger &operator=(const LuaDebugger &) = delete;

	void SetStopHandler(StopHandler handler);
	// Logpoint output. Defaults to Log::Info.
	void SetOutputHandler(OutputHandler handler);

	// Expects a function on top of L's stack and pops it. The function is
	// called with a userdata and returns a table of its fields (name ->
	// value) to show as its children, or nil. Assigning to a child assigns to
	// the userdata itself, so __newindex applies.
	void SetUserdataExpander(lua_State *L);

	// Replaces the breakpoints of source; an empty list removes them.
	void SetBreakpoints(std::string_view source, std::vector<LuaBreakpoint> breakpoints);
	void ClearAllBreakpoints();

	// Stop on errors that reach a message handler made by PushErrorHandler,
	// i.e. errors not caught by a Lua pcall.
	void SetBreakOnError(bool enabled);

	// Keeps an instruction-count hook on every thread so RequestPause can stop
	// code that never reaches a call, return or line hook (an endless loop).
	void SetPauseWatch(bool enabled);

	// Stops at the next line of Lua that runs. Safe from any OS thread.
	void RequestPause();
	// Withdraws a pause request no Lua has reached yet. Safe from any thread.
	void CancelPause();

	// Protected-call message handler: stops before unwinding for break-on-error, then
	// returns the error and traceback from the failing function.
	void PushErrorHandler(lua_State *L);

	bool IsStopped() const {
		return _stop.active;
	}

	// Records debugger Abort even if Lua catches its error.
	u64 AbortGeneration() const { return _abortGeneration; }

	// Inspection, valid only while stopped. Frame ids come from StackTrace;
	// 0 is the innermost frame.
	std::vector<LuaStackFrame> StackTrace();
	std::vector<LuaScope> Scopes(i32 frameId);
	std::vector<LuaVariable> Variables(i32 reference);
	LuaEvalResult Evaluate(i32 frameId, std::string_view expression);
	LuaEvalResult SetVariable(i32 reference, std::string_view name, std::string_view value);

  private:
	enum class HitOp {
		Always,
		GreaterEqual,
		Greater,
		Equal,
		Less,
		LessEqual,
		Modulo,
	};

	struct BreakpointState {
		LuaBreakpoint spec;
		HitOp hitOp = HitOp::Always;
		i64 hitOperand = 0;
		i64 hits = 0;
	};

	enum class RefKind {
		Locals,
		Upvalues,
		Globals,
		Value,
	};

	// A variables reference handed out during one stop.
	struct Ref {
		RefKind kind = RefKind::Locals;
		i32 level = 0;
		// Registry references (LUA_NOREF is -2): the table or userdata, and
		// the expander's table of a userdata's children once listed.
		int value = -2;
		int children = -2;
	};

	enum class StepMode {
		None,
		In,
		Over,
		Out,
	};

	struct StopContext {
		bool active = false;
		lua_State *thread = nullptr;
		// Stack depth of thread when it stopped; inspection that runs Lua
		// on the same thread shifts levels by the growth since.
		i32 depth = 0;
		// Level of frame id 0 (skips the message handler for an exception).
		i32 firstLevel = 0;
	};

	friend void LuaDebuggerHook(lua_State *L, lua_Debug *ar);
	friend int LuaDebuggerErrorHandler(lua_State *L);
	friend int LuaDebuggerCoroutineCreate(lua_State *L);
	friend int LuaDebuggerEnvIndex(lua_State *L);
	friend int LuaDebuggerEnvNewIndex(lua_State *L);

	// Returns true when the running Lua call must be aborted.
	bool onHook(lua_State *L, lua_Debug *ar);
	bool onLine(lua_State *L, lua_Debug *ar);
	LuaResumeAction stop(lua_State *L, LuaStopReason reason, i32 firstLevel, std::string message);
	void onError(lua_State *L);
	// L is the running thread that created thread.
	void registerThread(lua_State *L, lua_State *thread);

	// Stack work happens on the running thread: the stopped one while
	// stopped, otherwise the main thread (called between Lua calls).
	lua_State *workingThread() const;
	// Hook masks follow the debugger state; called after every change.
	// running is the thread currently executing, when called from a hook.
	void refreshHooks(lua_State *running = nullptr);
	void applyHook(lua_State *thread, bool lines);
	int baseMask() const;
	bool linesWanted() const;
	bool sourceHasBreakpoints(const char *source) const;
	bool currentFunctionHasBreakpoints(lua_State *thread);

	// Evaluates expression against the frame level of thread, which is
	// the running or stopped thread. Pushes the first result on success, the
	// error message on failure.
	bool evaluateAt(lua_State *thread, i32 level, std::string_view expression);
	void parseHitCondition(BreakpointState &breakpoint);
	bool breakpointApplies(lua_State *L, BreakpointState &breakpoint);
	std::string interpolate(lua_State *L, std::string_view message);
	void output(std::string_view text);

	i32 levelOf(i32 frameId) const;
	i32 addRef(Ref ref);
	void releaseRefs();
	LuaVariable describe(lua_State *L, int index, std::string name, i32 level);
	std::string format(lua_State *L, int index);

	lua_State *_main = nullptr;
	StopHandler _stopHandler;
	OutputHandler _outputHandler;
	int _expanderRef = -2; // LUA_NOREF

	std::map<std::string, std::vector<BreakpointState>, std::less<>> _breakpoints;
	bool _breakOnError = false;
	u64 _abortGeneration = 0;
	bool _pauseWatch = false;
	std::atomic<bool> _pauseRequested{false};

	StepMode _step = StepMode::None;
	lua_State *_stepThread = nullptr;
	i32 _stepDepth = 0;

	StopContext _stop;
	std::vector<Ref> _refs;
	// Breakpoint conditions and logpoints run Lua outside a stop; hooks on
	// any thread ignore that code.
	i32 _evaluating = 0;
};

} // namespace sowa
