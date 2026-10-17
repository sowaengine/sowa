#pragma once

#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <string_view>

#include "core/types.hpp"
#include "script/lua_debugger.hpp"

namespace sowa {

// Main-thread DAP session using request JSON and a send callback. Lua thread ID is 1;
// client paths map through localRoot or SetProjectRoot.
class DapSession {
  public:
	using Send = std::function<void(std::string json)>;

	struct Hooks {
		// Resume the current stop with action (ScriptSuspension::Request).
		std::function<void(LuaResumeAction)> resume;
		// The client asked to pause. Defaults to LuaDebugger::RequestPause.
		std::function<void()> pause;
	};

	DapSession(LuaDebugger &debugger, Send send, Hooks hooks);

	void SetProjectRoot(std::filesystem::path root);

	void Handle(std::string_view message);

	// Engine events.
	void OnStopped(const LuaStopEvent &event);
	// The stop ended on the engine's side (Continue in the editor, Stop).
	void OnContinued();
	// Logpoint output.
	void OnOutput(std::string_view text);
	// The connection closed without a disconnect request.
	void OnConnectionClosed();
	// The engine is shutting down.
	void OnTerminated();

	bool IsAttached() const {
		return _attached;
	}

	// configurationDone arrived since attaching: breakpoints are in place.
	bool IsConfigured() const {
		return _configured;
	}

	// "res://scripts/player.lua" for a file under the project root.
	std::optional<std::string> ToResource(std::string_view path) const;
	// The client path of a res:// source; empty when not mappable.
	std::string ToPath(std::string_view resource) const;

  private:
	void detach();
	void sendEvent(std::string_view event, const std::string &bodyJson);

	LuaDebugger &_debugger;
	Send _send;
	Hooks _hooks;
	std::filesystem::path _projectRoot;
	std::filesystem::path _localRoot;
	i64 _seq = 0;
	bool _attached = false;
	bool _configured = false;
};

} // namespace sowa
