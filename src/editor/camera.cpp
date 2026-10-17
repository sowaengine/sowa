#include "editor/camera.hpp"

#include <algorithm>

#include "math/matrix.hpp"

namespace sowa::editor {

Vector2 PanDelta(Vector2 windowDelta, f32 zoom, Vector2 gameSize, Vector2 imageSize) {
	if (imageSize.x < 1.f || imageSize.y < 1.f) {
		return {};
	}
	const f32 toGameX = gameSize.x / imageSize.x;
	const f32 toGameY = gameSize.y / imageSize.y;
	return Vector2(-windowDelta.x * toGameX * zoom, windowDelta.y * toGameY * zoom);
}

f32 ClampZoom(f32 zoom) {
	return std::clamp(zoom, kMinZoom, kMaxZoom);
}

f32 ClampScrollDelta(f32 delta) {
	return std::clamp(delta, -1.f, 1.f);
}

f32 ZoomAfterScroll(f32 zoom, f32 scrollY) {
	return ClampZoom(zoom - zoom * (scrollY * 0.1f));
}

Vector2 ZoomCursorPan(Vector2 imageMouse, Vector2 imageSize, Vector2 gameSize, f32 oldZoom,
	f32 newZoom) {
	if (imageSize.x < 1.f || imageSize.y < 1.f) {
		return {};
	}
	const f32 gameX = imageMouse.x * gameSize.x / imageSize.x;
	const f32 gameYDown = imageMouse.y * gameSize.y / imageSize.y;
	const Vector2 fromCenter(gameX - gameSize.x * 0.5f, gameSize.y * 0.5f - gameYDown);
	return fromCenter * (oldZoom - newZoom);
}

glm::mat4 ViewProjection(const EditorCamera &camera, Vector2 viewportSize) {
	const f32 zoom = std::max(ClampZoom(camera.zoom), kMinZoom);
	const glm::mat4 view =
		glm::inverse(Matrix::CalculateTransform2D(camera.position, 0.f, Vector2(zoom, zoom)));
	const f32 left = -viewportSize.x * 0.5f;
	const f32 bottom = -viewportSize.y * 0.5f;
	return Matrix::CalculateOrtho(left, left + viewportSize.x, bottom, bottom + viewportSize.y) *
		   view;
}

} // namespace sowa::editor
