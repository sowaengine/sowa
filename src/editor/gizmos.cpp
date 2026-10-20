#include "editor/gizmos.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

#include "ecs/components/transform2d.hpp"
#include "editor/viewport.hpp"
#include "physics/raycast2d.hpp"

namespace sowa::editor {
namespace {

struct GridIndexRange {
	i64 first = 0;
	i64 last = -1;
	bool valid = false;
};

GridIndexRange gridIndexRange(f32 first, f32 last, f32 spacing) {
	if (!std::isfinite(first) || !std::isfinite(last) || !std::isfinite(spacing) ||
		spacing <= 0.f || first > last) {
		return {};
	}

	const f64 firstIndex = std::ceil(static_cast<f64>(first) / spacing);
	const f64 lastIndex = std::floor(static_cast<f64>(last) / spacing);
	if (firstIndex > lastIndex || firstIndex < static_cast<f64>(std::numeric_limits<i64>::min()) ||
		lastIndex > static_cast<f64>(std::numeric_limits<i64>::max())) {
		return {};
	}
	return {static_cast<i64>(firstIndex), static_cast<i64>(lastIndex), true};
}

size_t nonAxisLineCount(const GridIndexRange &range) {
	if (!range.valid) {
		return 0;
	}
	const long double count = static_cast<long double>(range.last) -
							  static_cast<long double>(range.first) + 1.L -
							  (range.first <= 0 && range.last >= 0);
	if (count >= static_cast<long double>(std::numeric_limits<size_t>::max())) {
		return std::numeric_limits<size_t>::max();
	}
	return static_cast<size_t>(count);
}

} // namespace

Rect VisibleWorldBounds(const glm::mat4 &viewProjection) {
	const auto corners = ViewportWorldCorners(viewProjection);
	f32 minX = corners[0].x;
	f32 maxX = corners[0].x;
	f32 minY = corners[0].y;
	f32 maxY = corners[0].y;
	for (size_t i = 1; i < corners.size(); ++i) {
		minX = std::min(minX, corners[i].x);
		maxX = std::max(maxX, corners[i].x);
		minY = std::min(minY, corners[i].y);
		maxY = std::max(maxY, corners[i].y);
	}
	return Rect(minX, minY, maxX - minX, maxY - minY);
}

AdaptiveGrid AdaptiveGridLines(const Rect &visibleWorld, Vector2 viewportPixels,
	f32 targetMinorPixels, u32 subdivisions, size_t maxLines) {
	AdaptiveGrid grid;
	if (!std::isfinite(visibleWorld.x) || !std::isfinite(visibleWorld.y) ||
		!std::isfinite(visibleWorld.w) || !std::isfinite(visibleWorld.h) || visibleWorld.w <= 0.f ||
		visibleWorld.h <= 0.f || !std::isfinite(viewportPixels.x) ||
		!std::isfinite(viewportPixels.y) || viewportPixels.x <= 0.f || viewportPixels.y <= 0.f ||
		!std::isfinite(targetMinorPixels) || targetMinorPixels <= 0.f || subdivisions == 0 ||
		maxLines == 0) {
		return grid;
	}

	const f32 pixelsPerWorldUnit =
		std::min(viewportPixels.x / visibleWorld.w, viewportPixels.y / visibleWorld.h);
	const f32 desiredWorldSpacing = targetMinorPixels / pixelsPerWorldUnit;
	if (!std::isfinite(desiredWorldSpacing) || desiredWorldSpacing <= 0.f) {
		return grid;
	}

	// Powers of two keep common game coordinates (16, 32, 64, ...) aligned
	// while ensuring a zoom transition changes density by at most 2x.
	grid.minorSpacing = std::exp2(std::ceil(std::log2(desiredWorldSpacing)));
	if (!std::isfinite(grid.minorSpacing) || grid.minorSpacing <= 0.f) {
		return {};
	}

	const f32 maxX = visibleWorld.x + visibleWorld.w;
	const f32 maxY = visibleWorld.y + visibleWorld.h;
	GridIndexRange xRange;
	GridIndexRange yRange;
	for (;;) {
		xRange = gridIndexRange(visibleWorld.x, maxX, grid.minorSpacing);
		yRange = gridIndexRange(visibleWorld.y, maxY, grid.minorSpacing);
		const size_t xCount = nonAxisLineCount(xRange);
		const size_t yCount = nonAxisLineCount(yRange);
		if (xCount <= maxLines && yCount <= maxLines - xCount) {
			break;
		}
		grid.minorSpacing *= 2.f;
		if (!std::isfinite(grid.minorSpacing)) {
			return {};
		}
	}

	grid.majorSpacing = grid.minorSpacing * static_cast<f32>(subdivisions);
	if (!std::isfinite(grid.majorSpacing)) {
		return {};
	}

	grid.lines.reserve(nonAxisLineCount(xRange) + nonAxisLineCount(yRange));
	for (i64 index = xRange.first; xRange.valid;) {
		if (index != 0) {
			const f32 x = static_cast<f32>(index) * grid.minorSpacing;
			grid.lines.push_back({{Vector2(x, visibleWorld.y), Vector2(x, maxY)},
				index % static_cast<i64>(subdivisions) == 0 ? GridLineKind::Major
															: GridLineKind::Minor});
		}
		if (index == xRange.last) {
			break;
		}
		++index;
	}
	for (i64 index = yRange.first; yRange.valid;) {
		if (index != 0) {
			const f32 y = static_cast<f32>(index) * grid.minorSpacing;
			grid.lines.push_back({{Vector2(visibleWorld.x, y), Vector2(maxX, y)},
				index % static_cast<i64>(subdivisions) == 0 ? GridLineKind::Major
															: GridLineKind::Minor});
		}
		if (index == yRange.last) {
			break;
		}
		++index;
	}
	return grid;
}

OriginAxes VisibleOriginAxes(const Rect &visibleWorld) {
	OriginAxes axes;
	const f32 maxX = visibleWorld.x + visibleWorld.w;
	const f32 maxY = visibleWorld.y + visibleWorld.h;
	if (visibleWorld.y <= 0.f && maxY >= 0.f) {
		axes.x = GizmoLine{Vector2(visibleWorld.x, 0.f), Vector2(maxX, 0.f)};
	}
	if (visibleWorld.x <= 0.f && maxX >= 0.f) {
		axes.y = GizmoLine{Vector2(0.f, visibleWorld.y), Vector2(0.f, maxY)};
	}
	return axes;
}

bool IsInSceneTree(Entity entity, Entity sceneRoot) {
	return entity.IsAlive() && sceneRoot.IsAlive() &&
		   (entity == sceneRoot || entity.IsDescendantOf(sceneRoot));
}

std::optional<Vector2> SelectionWorldPosition(Entity selected) {
	if (!selected.IsAlive() || !selected.Has<Transform2D>()) {
		return std::nullopt;
	}
	const glm::mat4 world = ComputeWorldMatrix(selected);
	return Vector2(world[3][0], world[3][1]);
}

namespace {
bool finitePoint(Vector2 point) {
	return std::isfinite(point.x) && std::isfinite(point.y);
}

Vector2 mappedPoint(const glm::mat4 &matrix, Vector2 point) {
	const auto mapped = matrix * glm::vec4(point.x, point.y, 0.f, 1.f);
	return {mapped.x / mapped.w, mapped.y / mapped.w};
}

f32 dot(Vector2 a, Vector2 b) {
	return a.x * b.x + a.y * b.y;
}

f32 segmentDistance(Vector2 point, Vector2 a, Vector2 b) {
	const Vector2 delta = b - a;
	const f32 t = std::clamp(dot(point - a, delta) / delta.LengthSquared(), 0.f, 1.f);
	return point.DistanceTo(a + delta * t);
}
} // namespace

std::optional<GizmoLine> RaycastGizmoLine(Entity entity) {
	const auto *ray = entity.Get<Raycast2D>();
	if (ray == nullptr || !entity.Has<Transform2D>() || !finitePoint(ray->targetPosition)) {
		return {};
	}
	const auto matrix = ComputeWorldMatrix(entity);
	const Vector2 from = mappedPoint(matrix, {});
	const Vector2 to = mappedPoint(matrix, ray->targetPosition);
	return finitePoint(from) && finitePoint(to) ? std::optional<GizmoLine>{{from, to}}
												: std::nullopt;
}

std::optional<Vector2> RaycastTargetFromViewport(Entity entity, const glm::mat4 &viewProjection,
	const Rect &image, Vector2 mouse) {
	if (!RaycastGizmoLine(entity) || image.w <= 0.f || image.h <= 0.f || !finitePoint(mouse)) {
		return {};
	}
	const auto mapping = viewProjection * ComputeWorldMatrix(entity);
	const f32 determinant = glm::determinant(mapping);
	if (!std::isfinite(determinant) || std::abs(determinant) < 1.e-12f) {
		return {};
	}
	const Vector2 ndc{2.f * (mouse.x - image.x) / image.w - 1.f,
		1.f - 2.f * (mouse.y - image.y) / image.h};
	const auto target = mappedPoint(glm::inverse(mapping), ndc);
	return finitePoint(target) ? std::optional<Vector2>{target} : std::nullopt;
}

std::optional<GizmoGeometry> BuildGizmo(Entity entity, GizmoMode mode,
	const glm::mat4 &viewProjection, const Rect &image) {
	if (mode == GizmoMode::None || !entity.Has<Transform2D>() || image.w <= 0.f || image.h <= 0.f) {
		return std::nullopt;
	}
	const glm::mat4 parent = ComputeWorldMatrix(entity.GetParent());
	// A collapsed parent has no invertible mapping for editing its children.
	const f32 determinant = glm::determinant(parent);
	if (!std::isfinite(determinant) || std::abs(determinant) < 1e-8f) {
		return std::nullopt;
	}
	const auto &transform = *entity.Get<Transform2D>();
	const Vector2 worldOrigin = mappedPoint(parent, transform.position);
	const Vector2 origin = WorldToViewportImage(viewProjection, worldOrigin, image);
	Vector2 x(1.f, 0.f), y(0.f, 1.f);
	if (mode == GizmoMode::Scale) {
		const f32 angle = glm::radians(transform.rotation);
		const auto axisX = parent * glm::vec4(std::cos(angle), std::sin(angle), 0.f, 0.f);
		const auto axisY = parent * glm::vec4(-std::sin(angle), std::cos(angle), 0.f, 0.f);
		x = {axisX.x, axisX.y};
		y = {axisY.x, axisY.y};
	}
	x = WorldToViewportImage(viewProjection, worldOrigin + x, image) - origin;
	y = WorldToViewportImage(viewProjection, worldOrigin + y, image) - origin;
	if (!finitePoint(origin) || !finitePoint(x) || !finitePoint(y) || x.LengthSquared() < 1e-10f ||
		y.LengthSquared() < 1e-10f) {
		return std::nullopt;
	}
	return GizmoGeometry{origin, x.Normalized(), y.Normalized()};
}

GizmoHandle HitTestGizmo(const GizmoGeometry &geometry, GizmoMode mode, Vector2 mouse) {
	if (mode == GizmoMode::None) {
		return GizmoHandle::None;
	}
	const Vector2 delta = mouse - geometry.origin;
	if (mode == GizmoMode::Rotation) {
		const bool onRing = std::abs(delta.Length() - kGizmoRadius) <= 7.f;
		return onRing ? GizmoHandle::Ring : GizmoHandle::None;
	}
	if (std::abs(delta.x) <= 9.f && std::abs(delta.y) <= 9.f) {
		return GizmoHandle::Center;
	}
	const f32 xDistance = segmentDistance(mouse, geometry.origin + geometry.xDirection * 14.f,
		geometry.origin + geometry.xDirection * kGizmoLength);
	const f32 yDistance = segmentDistance(mouse, geometry.origin + geometry.yDirection * 14.f,
		geometry.origin + geometry.yDirection * kGizmoLength);
	if (std::min(xDistance, yDistance) > 8.f) {
		return GizmoHandle::None;
	}
	return xDistance <= yDistance ? GizmoHandle::X : GizmoHandle::Y;
}

std::optional<GizmoDrag> BeginGizmoDrag(Entity entity, GizmoMode mode, GizmoHandle handle,
	const glm::mat4 &viewProjection, const Rect &image, Vector2 mouse) {
	const auto geometry = BuildGizmo(entity, mode, viewProjection, image);
	if (!geometry || !finitePoint(mouse) || handle == GizmoHandle::None ||
		((mode == GizmoMode::Rotation) != (handle == GizmoHandle::Ring))) {
		return std::nullopt;
	}
	glm::mat4 screenToNdc(1.f);
	screenToNdc[0][0] = 2.f / image.w;
	screenToNdc[1][1] = -2.f / image.h;
	screenToNdc[3][0] = -1.f - 2.f * image.x / image.w;
	screenToNdc[3][1] = 1.f + 2.f * image.y / image.h;
	GizmoDrag drag{mode, handle, *entity.Get<Transform2D>(), *geometry};
	drag.screenToParent = glm::inverse(ComputeWorldMatrix(entity.GetParent())) *
						  glm::inverse(viewProjection) * screenToNdc;
	drag.startMouse = mouse;
	const Vector2 local = mappedPoint(drag.screenToParent, mouse) - drag.before.position;
	if (!finitePoint(local)) {
		return std::nullopt;
	}
	drag.lastAngle = std::atan2(local.y, local.x);
	return drag;
}

f32 GizmoScaleFactor(f32 pixels) {
	return std::exp(std::clamp(pixels / kGizmoLength, -10.f, 10.f));
}

Transform2D UpdateGizmoDrag(GizmoDrag &drag, Vector2 mouse) {
	Transform2D result = drag.before;
	if (drag.mode == GizmoMode::None || !finitePoint(mouse)) {
		return result;
	}
	Vector2 delta = mouse - drag.startMouse;
	if (drag.mode == GizmoMode::Position) {
		if (drag.handle == GizmoHandle::X) {
			delta = drag.geometry.xDirection * dot(delta, drag.geometry.xDirection);
		} else if (drag.handle == GizmoHandle::Y) {
			delta = drag.geometry.yDirection * dot(delta, drag.geometry.yDirection);
		}
		result.position += mappedPoint(drag.screenToParent, drag.startMouse + delta) -
						   mappedPoint(drag.screenToParent, drag.startMouse);
	} else if (drag.mode == GizmoMode::Rotation) {
		const Vector2 local = mappedPoint(drag.screenToParent, mouse) - drag.before.position;
		if (mouse.DistanceTo(drag.geometry.origin) > 3.f) {
			const f32 angle = std::atan2(local.y, local.x);
			drag.rotationDelta += std::remainder(angle - drag.lastAngle, glm::radians(360.f));
			drag.lastAngle = angle;
		}
		result.rotation += glm::degrees(drag.rotationDelta);
	} else if (drag.handle == GizmoHandle::Center) {
		const f32 factor = GizmoScaleFactor(delta.x - delta.y);
		result.scale *= factor;
	} else {
		f32 &scale = drag.handle == GizmoHandle::X ? result.scale.x : result.scale.y;
		const Vector2 direction =
			drag.handle == GizmoHandle::X ? drag.geometry.xDirection : drag.geometry.yDirection;
		const f32 distance = dot(delta, direction);
		scale = scale == 0.f ? distance / kGizmoLength : scale * GizmoScaleFactor(distance);
	}
	if (!finitePoint(result.position) || !finitePoint(result.scale) ||
		!std::isfinite(result.rotation)) {
		return drag.before;
	}
	return result;
}
} // namespace sowa::editor
