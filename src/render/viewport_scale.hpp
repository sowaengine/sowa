#pragma once

#include <optional>
#include <string_view>
#include <vector>

#include "core/reflection/enum_constant.hpp"
#include "core/types.hpp"
#include "math/rect.hpp"
#include "math/vector2.hpp"

namespace sowa {

// How the fixed game viewport is mapped into the OS window when presenting.
enum class ViewportScaleMode {
	Stretch,	// Fill the window (ignore aspect).
	KeepAspect, // Fit inside the window; letterbox/pillarbox (default).
	KeepWidth,	// Scale from window width; center vertically (may clip).
	KeepHeight, // Scale from window height; center horizontally (may clip).
	NoResize,	// 1:1 viewport pixels, centered (bars or clip).
};

const char *ViewportScaleModeToString(ViewportScaleMode mode);

// Parses names like "KeepAspect". Unknown -> KeepAspect and returns false.
bool TryParseViewportScaleMode(std::string_view name, ViewportScaleMode &out);

// Enum names for Archive::EnumField, derived from ViewportScaleModeToString
// to match TryParseViewportScaleMode.
const std::vector<EnumConstant> &ViewportScaleModeConstants();

// Computes the destination rect (in framebuffer pixels, origin bottom-left)
// for presenting a contentW x contentH viewport into a windowW x windowH
// drawable.
Rect CalculatePresentRect(i32 contentW, i32 contentH, i32 windowW, i32 windowH,
	ViewportScaleMode mode);

// Maps a GLFW cursor position (content coords, origin top-left, Y-down) into
// game-viewport coordinates (origin bottom-left, Y-up). presentRect is in
// framebuffer pixels.
Vector2 MapMouseToViewport(Vector2 windowMouse, Vector2 contentSize, Vector2 framebufferSize,
	const Rect &presentRect, Vector2 viewportSize);

struct ViewportPixel {
	i32 x = 0;
	i32 y = 0;
};

// Converts bottom-left-origin viewport coordinates to one valid texture
// texel. Exact right/top edges belong to the viewport and clamp to the final
// texel; points beyond them are outside.
std::optional<ViewportPixel> ViewportPointToPixel(Vector2 point, i32 width, i32 height);

} // namespace sowa
