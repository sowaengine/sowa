#pragma once

#include "core/types.hpp"
#include "resource/resource_id.hpp"

namespace sowa {

// Entity playback settings. AudioMixer owns voice cursors and playback state; the clip
// is a ResourceID so the audio callback never accesses component storage.
struct AudioSource {
	ResourceID clip;

	// Linear amplitude. 1 is the clip's own level. Pan and spatial scale
	// this per channel on the mixer.
	f32 gain = 1.f;

	// Playback rate scale. 1 is native. <= 0 does not advance.
	f32 pitch = 1.f;

	bool loop = false;

	// Start Playing when the mixer first sees this entity. Does not fire
	// again after the clip ends: the mixer keeps a Stopped voice.
	bool autoplay = false;

	// Stereo balance, -1 left, +1 right, 0 centre. Ignored when spatial
	// can resolve a Transform2D against a listener.
	f32 pan = 0.f;

	// When true, Sync writes pan and a distance scale from world position
	// versus the listener (the active camera). No Transform2D, or no
	// listener, plays centred at full gain.
	bool spatial = false;

	// Linear falloff radius in world units. 0 with spatial is silent.
	// Past this distance the voice is silent. Also the pan width: a source
	// this far on +x is hard-right.
	f32 maxDistance = 2000.f;
};

} // namespace sowa
