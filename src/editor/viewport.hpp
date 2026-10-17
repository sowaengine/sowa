#pragma once

#include <array>

#include <glm/glm.hpp>

#include "math/rect.hpp"
#include "math/vector2.hpp"

namespace sowa::editor {

// Fits a fixed-aspect game image into an ImGui panel. Origin is top-left
// (ImGui). Always KeepAspect - the editor viewport does not follow the
// project's window ScaleMode.
Rect FitGameImage(Vector2 gameSize, Vector2 panelSize);

// image is in window-content pixels (origin top-left, Y-down), matching
// GLFW cursor / ImGui without extra viewports. Result is framebuffer pixels
// (origin bottom-left) for Input::SetViewportMapping.
Rect GameImageToPresentRect(const Rect &image, Vector2 contentSize, Vector2 framebufferSize);

// The four world-space points visible through viewProjection, ordered
// bottom-left, bottom-right, top-right, top-left. Unprojecting the actual
// camera matrix keeps editor outlines faithful to Camera2D zoom, rotation,
// offset, anchor and inherited transforms.
std::array<Vector2, 4> ViewportWorldCorners(const glm::mat4 &viewProjection);

// Projects a world-space point into the on-screen game image. image uses
// ImGui's top-left, Y-down coordinates; the view-projection uses OpenGL's
// bottom-left, Y-up convention.
Vector2 WorldToViewportImage(const glm::mat4 &viewProjection, Vector2 worldPoint,
	const Rect &image);

} // namespace sowa::editor
