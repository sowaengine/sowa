#pragma once

#include "core/types.hpp"
#include "math/color.hpp"
#include "math/rect.hpp"
#include "math/vector2.hpp"
#include "resource/resource_id.hpp"

namespace sowa {

// Textured quad at an entity's transform. Registered by SpriteRenderer2D so World
// does not depend on rendering types.
struct Sprite {
	// ImageTexture ID, resolved by SpriteRenderer2D each frame.
	ResourceID texture;

	// Tint multiplied into the sampled texel. Opaque white leaves it alone.
	Color modulate;

	// Local quad size in pixels. Zero uses texture dimensions; explicit size stretches the image.
	Vector2 size{0.f, 0.f};

	// Local offset; rotation and scale still use the entity origin.
	Vector2 offset{0.f, 0.f};

	// Centers the quad on the origin; otherwise it grows right and up.
	bool centered = true;

	// Uses the texture size when both size axes are zero.
	Vector2 ResolvedSize(Vector2 textureSize) const {
		return size.x == 0.f && size.y == 0.f ? textureSize : size;
	}

	// Local quad with centered and offset applied. Pass the resolved texture size.
	Rect LocalRect(Vector2 resolvedSize) const {
		const f32 originX = centered ? offset.x - resolvedSize.x * 0.5f : offset.x;
		const f32 originY = centered ? offset.y - resolvedSize.y * 0.5f : offset.y;
		return Rect(originX, originY, resolvedSize.x, resolvedSize.y);
	}
};

} // namespace sowa
