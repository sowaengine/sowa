#pragma once

#include "core/types.hpp"

namespace sowa {

// Game time for this frame, computed by Application and shared by play-only systems,
// Lua timers and tweens, World::Progress, and runtime UI input.
struct GameTime {
	// Game delta for this frame. 0 while paused.
	f32 delta = 0.f;
	// Gameplay runs this frame: play-only systems, timers/tweens, UI input.
	bool running = false;
};

// Stopped: editor time - frame delta, no gameplay. Playing: frame delta.
// Paused: nothing advances. A step frame while paused advances exactly
// stepDelta, the physics fixed delta, so one step is one physics tick.
inline GameTime GetGameTime(bool playing, bool paused, bool step, f32 frameDelta, f32 stepDelta) {
	if (!playing) {
		return {frameDelta, false};
	}
	if (!paused) {
		return {frameDelta, true};
	}
	if (step) {
		return {stepDelta, true};
	}
	return {0.f, false};
}

} // namespace sowa
