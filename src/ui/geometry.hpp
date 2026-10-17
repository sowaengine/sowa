#pragma once

#include "ui/components.hpp"

namespace sowa {
Vector2 UITransformPoint(const glm::mat4 &matrix, Vector2 point);
std::array<Vector2, 4> UIRectCorners(Rect rect, const glm::mat4 &matrix);
Entity FindUICanvas(Entity entity);
bool UpdateUICanvasMapping(Entity entity, Vector2 viewport, const glm::mat4 &worldViewProjection);

// Resize from a normalized corner (0/1 on each axis), keeping the opposite
// corner fixed. Delta and resolved bounds are in canvas layout coordinates.
UIRect ResizeUIRect(const UIRect &before, Vector2 delta, Vector2 corner, f32 unitScale);
} // namespace sowa
