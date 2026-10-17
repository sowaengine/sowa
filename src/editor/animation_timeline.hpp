#pragma once

#include <functional>

#include "animation/animation.hpp"

namespace sowa::editor {

// A reusable ImGui timeline: rows, key diamonds, a ruler and one key inspector.
// Returns true only for authored edits. Scrubbing only updates the cursor.
class AnimationTimeline {
  public:
	using ResourceEditor = std::function<bool(ResourceID &, const AnimationBinding &)>;

	bool Draw(World &world, Entity entity, AnimationClip &clip, f64 &cursor,
		const ResourceEditor &editResource = {});
	void Reset();

	bool DidScrub() const {
		return _scrubbed;
	}

  private:
	bool drawTracks(World &world, Entity entity, AnimationClip &clip, f64 &cursor);
	bool drawSelectedKey(AnimationClip &clip, const ResourceEditor &editResource);
	bool drawAddTrack(World &world, Entity entity, AnimationClip &clip, f64 cursor);

	i32 _track = -1;
	u64 _key = 0;
	i32 _event = -1;
	u64 _dragKey = 0;
	i32 _dragTrack = -1;
	i32 _dragEvent = -1;
	f64 _dragTime = 0;
	f32 _snap = .1f;
	f32 _zoom = 1.f;
	std::string _error;
	bool _scrubbed = false;
};

} // namespace sowa::editor
