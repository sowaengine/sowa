#pragma once

#include <string_view>

#include "scene/scene_manager.hpp"

namespace sowa::editor {

enum class PlayControl {
	PlayProject,
	PlayCurrent,
	Stop,
	Pause,
	Resume,
	Step,
};

struct PlayControlAvailability {
	bool playProject = false;
	bool playCurrent = false;
	bool stop = false;
	bool pause = false;
	bool resume = false;
	bool step = false;
};

// Toolbar state: project play needs a startup scene; current play needs an editor document.
// Pause needs a running session. Resume and Step need a paused session; Step is unavailable
// while a debugger holds execution inside a frame.
PlayControlAvailability AvailablePlayControls(PlayKind mode, bool paused, bool hasCurrentScene,
	bool hasMainScene, bool scriptSuspended = false);

// Queues the selected SceneManager operation if it is currently available.
// Play/Stop are applied at the next Flush; Pause/Resume are immediate and
// Step is taken by the next frame. Toolbar and F5–F9 hotkeys both use this.
bool RequestPlayControl(SceneManager &scenes, PlayControl control, std::string_view mainScene);

} // namespace sowa::editor
