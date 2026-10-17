#pragma once

#include "core/types.hpp"

namespace sowa {

class Vector2;

class Rect {
  public:
	f32 x = 0.f;
	f32 y = 0.f;
	f32 w = 0.f;
	f32 h = 0.f;

	Rect() = default;

	Rect(f32 x, f32 y) : x(x), y(y), w(0.f), h(0.f) {
	}

	Rect(f32 x, f32 y, f32 w, f32 h) : x(x), y(y), w(w), h(h) {
	}

	~Rect() = default;

	f32 Left() const {
		return x;
	}

	f32 Right() const {
		return x + w;
	}

	f32 Bottom() const {
		return y;
	}

	f32 Top() const {
		return y + h;
	}

	// Remaps point from this rect's coordinate space into target's.
	Vector2 MapPoint(const Vector2 &point, const Rect &target) const;

	bool Contains(const Vector2 &point) const;
	bool Intersects(const Rect &other) const;
};

} // namespace sowa
