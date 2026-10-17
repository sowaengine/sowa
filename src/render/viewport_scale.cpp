#include "render/viewport_scale.hpp"

#include <algorithm>
#include <cmath>

#include "math/vector2.hpp"

namespace sowa {

const char *ViewportScaleModeToString(ViewportScaleMode mode) {
	switch (mode) {
	case ViewportScaleMode::Stretch:
		return "Stretch";
	case ViewportScaleMode::KeepAspect:
		return "KeepAspect";
	case ViewportScaleMode::KeepWidth:
		return "KeepWidth";
	case ViewportScaleMode::KeepHeight:
		return "KeepHeight";
	case ViewportScaleMode::NoResize:
		return "NoResize";
	}
	return "KeepAspect";
}

bool TryParseViewportScaleMode(std::string_view name, ViewportScaleMode &out) {
	if (name == "Stretch") {
		out = ViewportScaleMode::Stretch;
		return true;
	}
	if (name == "KeepAspect") {
		out = ViewportScaleMode::KeepAspect;
		return true;
	}
	if (name == "KeepWidth") {
		out = ViewportScaleMode::KeepWidth;
		return true;
	}
	if (name == "KeepHeight") {
		out = ViewportScaleMode::KeepHeight;
		return true;
	}
	if (name == "NoResize") {
		out = ViewportScaleMode::NoResize;
		return true;
	}
	out = ViewportScaleMode::KeepAspect;
	return false;
}

const std::vector<EnumConstant> &ViewportScaleModeConstants() {
	static const std::vector<EnumConstant> constants = [] {
		std::vector<EnumConstant> built;
		for (const ViewportScaleMode mode : {ViewportScaleMode::Stretch,
				 ViewportScaleMode::KeepAspect, ViewportScaleMode::KeepWidth,
				 ViewportScaleMode::KeepHeight, ViewportScaleMode::NoResize}) {
			built.push_back(EnumConstant{ViewportScaleModeToString(mode), static_cast<i32>(mode)});
		}
		return built;
	}();
	return constants;
}

Rect CalculatePresentRect(i32 contentW, i32 contentH, i32 windowW, i32 windowH,
	ViewportScaleMode mode) {
	contentW = std::max(contentW, 1);
	contentH = std::max(contentH, 1);
	windowW = std::max(windowW, 1);
	windowH = std::max(windowH, 1);

	const f32 cW = static_cast<f32>(contentW);
	const f32 cH = static_cast<f32>(contentH);
	const f32 wW = static_cast<f32>(windowW);
	const f32 wH = static_cast<f32>(windowH);

	switch (mode) {
	case ViewportScaleMode::Stretch:
		return Rect(0.f, 0.f, wW, wH);

	case ViewportScaleMode::KeepAspect: {
		const f32 windowRatio = wW / wH;
		const f32 contentRatio = cW / cH;
		if (windowRatio > contentRatio) {
			const f32 height = wH;
			const f32 width = height * contentRatio;
			return Rect((wW - width) * 0.5f, 0.f, width, height);
		}
		const f32 width = wW;
		const f32 height = width / contentRatio;
		return Rect(0.f, (wH - height) * 0.5f, width, height);
	}

	case ViewportScaleMode::KeepWidth: {
		const f32 scale = wW / cW;
		const f32 width = wW;
		const f32 height = cH * scale;
		return Rect(0.f, (wH - height) * 0.5f, width, height);
	}

	case ViewportScaleMode::KeepHeight: {
		const f32 scale = wH / cH;
		const f32 height = wH;
		const f32 width = cW * scale;
		return Rect((wW - width) * 0.5f, 0.f, width, height);
	}

	case ViewportScaleMode::NoResize:
		return Rect((wW - cW) * 0.5f, (wH - cH) * 0.5f, cW, cH);
	}

	return Rect(0.f, 0.f, wW, wH);
}

Vector2 MapMouseToViewport(Vector2 windowMouse, Vector2 contentSize, Vector2 framebufferSize,
	const Rect &presentRect, Vector2 viewportSize) {
	contentSize.x = std::max(contentSize.x, 1.f);
	contentSize.y = std::max(contentSize.y, 1.f);
	framebufferSize.x = std::max(framebufferSize.x, 1.f);
	framebufferSize.y = std::max(framebufferSize.y, 1.f);

	const f32 scaleX = framebufferSize.x / contentSize.x;
	const f32 scaleY = framebufferSize.y / contentSize.y;

	Vector2 fbPos(windowMouse.x * scaleX, windowMouse.y * scaleY);
	// GLFW Y-down → framebuffer Y-up.
	fbPos.y = framebufferSize.y - fbPos.y;

	const Rect viewportRect(0.f, 0.f, viewportSize.x, viewportSize.y);
	return presentRect.MapPoint(fbPos, viewportRect);
}

std::optional<ViewportPixel> ViewportPointToPixel(Vector2 point, i32 width, i32 height) {
	if (width <= 0 || height <= 0 || !std::isfinite(point.x) || !std::isfinite(point.y) ||
		point.x < 0.f || point.y < 0.f || point.x > static_cast<f32>(width) ||
		point.y > static_cast<f32>(height)) {
		return std::nullopt;
	}

	return ViewportPixel{std::min(static_cast<i32>(std::floor(point.x)), width - 1),
		std::min(static_cast<i32>(std::floor(point.y)), height - 1)};
}

} // namespace sowa
