#pragma once

#include "math/vector2.hpp"

namespace sowa {

class World;

// A render-only layer on this entity and its descendants. The nearest
// Parallax2D wins; nested layers do not multiply their camera speeds.
struct Parallax2D {
	bool tileX = false;
	bool tileY = false;

	// Apparent movement relative to the camera: 1 = ordinary world content,
	// 0 = fixed relative to camera translation, 0.5 = half-speed background.
	f32 xSpeed = 0.5f;
	f32 ySpeed = 0.5f;

	// Extra world-space displacement; scripts can update this each frame.
	Vector2 offset{0.f, 0.f};

	bool limitX = false;
	bool limitY = false;
	// Fixed world-space edges of the tiled area. Partial tiles are clipped.
	// An enabled axis with min >= max draws nothing.
	Vector2 limitMin{0.f, 0.f};
	Vector2 limitMax{0.f, 0.f};
};

void RegisterParallax2D(World &world);

} // namespace sowa
