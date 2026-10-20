#include "editor/viewport.hpp"
#include <algorithm>
#include <cmath>

#include <algorithm>

#include <glm/gtc/matrix_inverse.hpp>

#include "render/viewport_scale.hpp"

namespace sowa::editor {

Rect FitGameImage(Vector2 gameSize, Vector2 panelSize) {
	const i32 gameW = std::max(1, static_cast<i32>(gameSize.x + 0.5f));
	const i32 gameH = std::max(1, static_cast<i32>(gameSize.y + 0.5f));
	const i32 panelW = std::max(1, static_cast<i32>(panelSize.x + 0.5f));
	const i32 panelH = std::max(1, static_cast<i32>(panelSize.y + 0.5f));
	return CalculatePresentRect(gameW, gameH, panelW, panelH, ViewportScaleMode::KeepAspect);
}

Rect GameImageToPresentRect(const Rect &image, Vector2 contentSize, Vector2 framebufferSize) {
	contentSize.x = std::max(contentSize.x, 1.f);
	contentSize.y = std::max(contentSize.y, 1.f);
	framebufferSize.x = std::max(framebufferSize.x, 1.f);
	framebufferSize.y = std::max(framebufferSize.y, 1.f);
	const f32 scaleX = framebufferSize.x / contentSize.x;
	const f32 scaleY = framebufferSize.y / contentSize.y;
	return Rect(image.x * scaleX, framebufferSize.y - (image.y + image.h) * scaleY,
		image.w * scaleX, image.h * scaleY);
}

std::array<Vector2, 4> ViewportWorldCorners(const glm::mat4 &viewProjection) {
	const glm::mat4 inverse = glm::inverse(viewProjection);
	const glm::vec2 ndcCorners[] = {
		{-1.f, -1.f},
		{1.f, -1.f},
		{1.f, 1.f},
		{-1.f, 1.f},
	};
	std::array<Vector2, 4> result;
	for (size_t i = 0; i < result.size(); ++i) {
		const glm::vec4 world = inverse * glm::vec4(ndcCorners[i].x, ndcCorners[i].y, 0.f, 1.f);
		const f32 w = world.w != 0.f ? world.w : 1.f;
		result[i] = Vector2(world.x / w, world.y / w);
	}
	return result;
}

Vector2 WorldToViewportImage(const glm::mat4 &viewProjection, Vector2 worldPoint,
	const Rect &image) {
	const glm::vec4 clip = viewProjection * glm::vec4(worldPoint.x, worldPoint.y, 0.f, 1.f);
	const f32 w = clip.w != 0.f ? clip.w : 1.f;
	const f32 ndcX = clip.x / w;
	const f32 ndcY = clip.y / w;
	return Vector2(image.x + (ndcX * 0.5f + 0.5f) * image.w,
		image.y + (0.5f - ndcY * 0.5f) * image.h);
}

std::optional<PickRectangle> BoxPickRectangle(Vector2 a, Vector2 b, Vector2 size) {
	if (!std::isfinite(a.x) || !std::isfinite(a.y) || !std::isfinite(b.x) || !std::isfinite(b.y) ||
		!std::isfinite(size.x) || !std::isfinite(size.y) || size.x < 1 || size.y < 1 ||
		size.x > 32768 || size.y > 32768) {
		return {};
	}
	const auto x0 = static_cast<i32>(std::floor(std::clamp(std::min(a.x, b.x), 0.f, 1.f) * size.x));
	const auto x1 = static_cast<i32>(std::ceil(std::clamp(std::max(a.x, b.x), 0.f, 1.f) * size.x));
	const auto y0 =
		static_cast<i32>(std::floor((1.f - std::clamp(std::max(a.y, b.y), 0.f, 1.f)) * size.y));
	const auto y1 =
		static_cast<i32>(std::ceil((1.f - std::clamp(std::min(a.y, b.y), 0.f, 1.f)) * size.y));
	if (x1 <= x0 || y1 <= y0) {
		return {};
	}
	return PickRectangle{x0, y0, x1 - x0, y1 - y0};
}
} // namespace sowa::editor
