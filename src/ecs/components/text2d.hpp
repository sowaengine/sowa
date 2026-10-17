#pragma once

#include <string>

#include "math/color.hpp"
#include "render/text_layout.hpp"
#include "resource/resource_id.hpp"

namespace sowa {

// World-space text. Transform2D supplies its hierarchy, rotation and scale;
// RenderOrder and Hidden work just as they do for other 2D visuals.
struct Text2D {
	ResourceID font;
	std::string text = "Text";
	Color modulate;
	f32 fontSize = 0.f;
	f32 wrapWidth = 0.f;
	f32 lineSpacing = 1.f;
	TextHorizontalAlignment horizontalAlignment = TextHorizontalAlignment::Left;
	TextVerticalAlignment verticalAlignment = TextVerticalAlignment::Baseline;
	Vector2 offset{0.f, 0.f};

	TextLayoutOptions LayoutOptions() const {
		return {fontSize, wrapWidth, lineSpacing, horizontalAlignment, verticalAlignment, offset};
	}
};

} // namespace sowa
