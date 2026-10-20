#pragma once

#include <cstddef>
#include <optional>
#include <vector>

#include <glm/glm.hpp>

#include "core/types.hpp"
#include "ecs/components/transform2d.hpp"
#include "ecs/entity.hpp"
#include "math/rect.hpp"
#include "math/vector2.hpp"

namespace sowa::editor {

struct GizmoLine {
	Vector2 a;
	Vector2 b;
};

// Authored ray preview and inverse mapping for dragging its local endpoint.
std::optional<GizmoLine> RaycastGizmoLine(Entity entity);
std::optional<Vector2> RaycastTargetFromViewport(Entity entity, const glm::mat4 &viewProjection,
	const Rect &image, Vector2 mouse);

struct OriginAxes {
	std::optional<GizmoLine> x;
	std::optional<GizmoLine> y;
};

enum class GridLineKind {
	Minor,
	Major,
};

struct AdaptiveGridLine {
	GizmoLine line;
	GridLineKind kind = GridLineKind::Minor;
};

struct AdaptiveGrid {
	f32 minorSpacing = 0.f;
	f32 majorSpacing = 0.f;
	std::vector<AdaptiveGridLine> lines;
};

inline constexpr f32 kTargetMinorGridPixels = 24.f;
inline constexpr u32 kGizmoGridSubdivisions = 4;
inline constexpr size_t kMaxGizmoGridLines = 256;

// Axis-aligned world bounds visible through the editor camera. The editor
// camera currently has no rotation, but deriving this from its projection
// keeps pan and zoom in one source of truth.
Rect VisibleWorldBounds(const glm::mat4 &viewProjection);

// World grid with power-of-two spacing, major lines every four cells, and a bounded
// line count. Origin axes are drawn separately.
AdaptiveGrid AdaptiveGridLines(const Rect &visibleWorld, Vector2 viewportPixels,
	f32 targetMinorPixels = kTargetMinorGridPixels, u32 subdivisions = kGizmoGridSubdivisions,
	size_t maxLines = kMaxGizmoGridLines);

// Returns only axes that cross the visible world rectangle. They stay named
// so an off-screen X axis cannot make the visible Y axis inherit X's colour.
OriginAxes VisibleOriginAxes(const Rect &visibleWorld);

// Checks scene subtree membership, including nested prefab roots.
bool IsInSceneTree(Entity entity, Entity sceneRoot);

// The selected entity's current world-space transform origin. Requires a
// Transform2D on the entity itself; parents are still composed so the marker
// agrees with the rendered object even before the transform system runs.
std::optional<Vector2> SelectionWorldPosition(Entity selected);

enum class GizmoMode {
	None,
	Position,
	Rotation,
	Scale
};
enum class GizmoHandle {
	None,
	X,
	Y,
	Center,
	Ring
};

inline constexpr f32 kGizmoLength = 72.f;
inline constexpr f32 kGizmoRadius = 56.f;

// Multiplicative dragging keeps mirrored scales mirrored and cannot cross zero.
f32 GizmoScaleFactor(f32 pixels);

// Screen-space handles stay the same size at every camera zoom. Position
// uses world axes; scale uses the entity's rotated axes, including parents.
struct GizmoGeometry {
	Vector2 origin;
	Vector2 xDirection;
	Vector2 yDirection;
};

std::optional<GizmoGeometry> BuildGizmo(Entity entity, GizmoMode mode,
	const glm::mat4 &viewProjection, const Rect &image);
GizmoHandle HitTestGizmo(const GizmoGeometry &geometry, GizmoMode mode, Vector2 mouse);

// Capture the camera and parent mapping at mouse-down so updates never
// accumulate translation/scale error or depend on cached world matrices.
struct GizmoDrag {
	GizmoMode mode;
	GizmoHandle handle;
	Transform2D before;
	GizmoGeometry geometry;
	glm::mat4 screenToParent{1.f};
	Vector2 startMouse;
	f32 lastAngle = 0.f;
	f32 rotationDelta = 0.f;
};

std::optional<GizmoDrag> BeginGizmoDrag(Entity entity, GizmoMode mode, GizmoHandle handle,
	const glm::mat4 &viewProjection, const Rect &image, Vector2 mouse);
Transform2D UpdateGizmoDrag(GizmoDrag &drag, Vector2 mouse);

} // namespace sowa::editor
