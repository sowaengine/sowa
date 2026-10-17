#pragma once

#include "ecs/components/sprite.hpp"
#include <string>

namespace sowa {

// Authored settings are serialized; playback state belongs to each entity.
struct AnimatedSprite {
	ResourceID animations;
	std::string currentAnimation;
	bool autoplay = true;
	f32 speed = 1.f;
	Color modulate;
	Vector2 size{0, 0};
	Vector2 offset{0, 0};
	bool centered = true;

	bool isPlaying = false;
	i32 frame = 0;

	struct Playback {
		ResourceID animations;
		std::string animation;
		f64 time = 0;
		bool initialized = false;
	} playback;

	Rect LocalRect(Vector2 frameSize) const {
		Sprite sprite;
		sprite.size = size;
		sprite.offset = offset;
		sprite.centered = centered;
		return sprite.LocalRect(sprite.ResolvedSize(frameSize));
	}
};

} // namespace sowa
