#include "script/script_suspension.hpp"

#include "audio/audio_mixer.hpp"
#include "scene/scene_manager.hpp"

namespace sowa {

ScriptSuspension::ScriptSuspension(SceneManager &scenes, AudioMixer &mixer)
	: _scenes(scenes), _mixer(mixer) {
}

bool ScriptSuspension::WouldSuspend() const {
	return (_scenes.IsPlaying() || _suspendOutsidePlay) && !_scenes.IsStopPending();
}

LuaResumeAction ScriptSuspension::Run(const LuaStopEvent &event,
	const std::function<bool()> &pumpFrame) {
	if (!WouldSuspend()) {
		return _scenes.IsStopPending() ? LuaResumeAction::Abort : LuaResumeAction::Continue;
	}

	// Outside Play (no editor) there is no session to pause or resume.
	const bool playing = _scenes.IsPlaying();
	const bool wasPaused = _scenes.IsPaused();
	const bool mixerWasPaused = _mixer.IsPaused();
	_scenes.Pause();
	_scenes.SetScriptSuspended(true);
	_mixer.SetPaused(true);
	_event = event;
	_active = true;

	LuaResumeAction action = LuaResumeAction::Continue;
	bool restorePause = true;
	while (true) {
		if (!pumpFrame()) {
			action = LuaResumeAction::Abort;
			break;
		}
		if (_scenes.IsStopPending()) {
			action = LuaResumeAction::Abort;
			break;
		}
		if (_requested) {
			action = *_requested;
			break;
		}
		if (playing && !_scenes.IsPaused()) {
			// Resumed from the toolbar: the user asked for a running session.
			restorePause = false;
			break;
		}
	}

	_active = false;
	// A request belongs to this stop only; it must not end the next one.
	_requested.reset();
	_scenes.SetScriptSuspended(false);
	if (restorePause && !wasPaused && action != LuaResumeAction::Abort) {
		_scenes.Resume();
	}
	_mixer.SetPaused(restorePause ? mixerWasPaused : false);
	return action;
}

void ScriptSuspension::Request(LuaResumeAction action) {
	if (_active) {
		_requested = action;
	}
}

} // namespace sowa
