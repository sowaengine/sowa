#pragma once

#include <glm/glm.hpp>

#include "core/types.hpp"
#include "math/vector2.hpp"

namespace sowa::editor {

// Independent of the scene's Camera2D. Edit mode looks through this;
// play mode uses the scene camera again. Larger zoom is a wider view
// (Transform2D scale), matching Camera2D - there is no separate zoom field.
struct EditorCamera {
	Vector2 position{0.f, 0.f};
	f32 zoom = 1.f;
};

constexpr f32 kMinZoom = 0.1f;
constexpr f32 kMaxZoom = 100.f;

// windowDelta is content pixels, Y-down (ImGui / GLFW). Result is a
// world-space move of the camera, Y-up, so the world follows the drag
// (grab the canvas). imageSize is the on-screen game image so letterbox
// scale is not treated as 1:1 with the viewport.
Vector2 PanDelta(Vector2 windowDelta, f32 zoom, Vector2 gameSize, Vector2 imageSize);

// macOS trackpads report large GLFW wheel values in one frame. Clamp to
// one notch so panel scroll and viewport zoom stay usable.
f32 ClampScrollDelta(f32 delta);

// scrollY > 0 zooms in (smaller zoom). Clamped to [kMinZoom, kMaxZoom].
f32 ZoomAfterScroll(f32 zoom, f32 scrollY);

f32 ClampZoom(f32 zoom);

// Camera pan that keeps the world point under the cursor fixed when zoom
// changes. imageMouse is pixels from the top-left of the on-screen game
// image, Y-down. Result is Y-up world space.
Vector2 ZoomCursorPan(Vector2 imageMouse, Vector2 imageSize, Vector2 gameSize, f32 oldZoom,
	f32 newZoom);

// Same framing as a Camera2D entity at camera.position with scale
// (zoom, zoom), rotation 0, offset 0, anchor (0.5, 0.5).
glm::mat4 ViewProjection(const EditorCamera &camera, Vector2 viewportSize);

} // namespace sowa::editor
