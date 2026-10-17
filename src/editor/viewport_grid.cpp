#include "ecs/components/camera2d.hpp"
#include "editor/editor.hpp"
#include "editor/viewport.hpp"
#include "render/renderer2d.hpp"

namespace sowa {

void Editor::DrawViewportGrid(Renderer2D &renderer, Vector2 viewportSize) {
	const auto projection = editor::ViewProjection(_camera, viewportSize);
	const Rect bounds = editor::VisibleWorldBounds(projection);
	const Vector2 pixels = _gameImageRect.w > 0 && _gameImageRect.h > 0
							   ? Vector2(_gameImageRect.w, _gameImageRect.h)
							   : viewportSize;
	const f32 unit = bounds.w / pixels.x;
	const auto grid = editor::AdaptiveGridLines(bounds, pixels);
	renderer.Begin();
	renderer.SetAlphaCutoff(0);
	for (const auto &line : grid.lines) {
		const bool major = line.kind == editor::GridLineKind::Major;
		const Color color{205.f / 255, 205.f / 255, 210.f / 255, (major ? 48.f : 22.f) / 255};
		renderer.DrawLine(line.line.a, line.line.b, color, unit * (major ? 1.15f : 1.f));
	}
	const auto axes = editor::VisibleOriginAxes(bounds);
	if (axes.x) {
		renderer.DrawLine(axes.x->a, axes.x->b, {205.f / 255, 95.f / 255, 85.f / 255, 190.f / 255},
			unit * 1.5f);
	}
	if (axes.y) {
		renderer.DrawLine(axes.y->a, axes.y->b, {90.f / 255, 185.f / 255, 110.f / 255, 190.f / 255},
			unit * 1.5f);
	}
	const auto outline = editor::ViewportWorldCorners(CalculateBlankViewProjection(viewportSize));
	for (size_t i = 0; i < outline.size(); ++i) {
		renderer.DrawLine(outline[i], outline[(i + 1) % outline.size()],
			{214.f / 255, 214.f / 255, 220.f / 255, 230.f / 255}, unit * 1.5f);
	}
	renderer.End();
	renderer.SetAlphaCutoff(.1f);
}

} // namespace sowa
