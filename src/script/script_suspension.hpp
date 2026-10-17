#pragma once

#include <functional>
#include <optional>

#include "script/lua_debugger.hpp"

namespace sowa {

class AudioMixer;
class SceneManager;

// Pauses game time and audio while pumping the editor at a breakpoint. No scene flush,
// script reload, or world advance. Stop/close aborts Lua before teardown; Continue
// restores the preceding pause state.
class ScriptSuspension {
  public:
	ScriptSuspension(SceneManager &scenes, AudioMixer &mixer);

	// Whether Run would hold a stop now rather than return at once: during a
	// play session, or at any time when SetSuspendOutsidePlay is on; and
	// not with a Stop already queued (Run aborts then).
	bool WouldSuspend() const;

	// The editor suspends only during Play to prevent world edits during tool-system
	// execution. Run mode and debug templates also allow startup-script debugging.
	void SetSuspendOutsidePlay(bool enabled) {
		_suspendOutsidePlay = enabled;
	}

	// Returns Continue (or Abort, with a Stop queued) at once when
	// WouldSuspend is false.
	LuaResumeAction Run(const LuaStopEvent &event, const std::function<bool()> &pumpFrame);

	// Resumes after the current pumped frame. Ignored when not suspended.
	void Request(LuaResumeAction action);

	bool IsActive() const {
		return _active;
	}

	// The stop being suspended on; valid while IsActive.
	const LuaStopEvent &Event() const {
		return _event;
	}

  private:
	SceneManager &_scenes;
	AudioMixer &_mixer;
	bool _active = false;
	bool _suspendOutsidePlay = false;
	LuaStopEvent _event;
	std::optional<LuaResumeAction> _requested;
};

} // namespace sowa
