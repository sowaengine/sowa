#pragma once

#include "math/color.hpp"
#include "math/rect.hpp"
#include "math/vector2.hpp"
#include "resource/resource_id.hpp"

namespace sowa {

// A resizable textured rectangle. Padding defines the fixed edges in texture pixels.
struct NinePatchRect {
	ResourceID texture;
	f32 paddingLeft = 8;
	f32 paddingRight = 8;
	f32 paddingTop = 8;
	f32 paddingBottom = 8;
	bool tileCenter = false;
	bool tileEdges = false;
	bool drawCenter = true;
	Color modulate;
	Vector2 size{160, 100};
	Vector2 offset{0, 0};
	bool centered = true;

	Rect LocalRect() const {
		const Vector2 origin = centered ? offset - size * .5f : offset;
		return {origin.x, origin.y, size.x, size.y};
	}
};

} // namespace sowa
