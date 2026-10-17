#include "ui/drawing.hpp"
#include "render/image_patches.hpp"
#include "render/renderer2d.hpp"
#include "resource/image_texture.hpp"
#include "resource/resource_registry.hpp"
#include <algorithm>

namespace sowa {
namespace {

void drawPatches(Renderer2D &renderer, const ImageTexture &texture, Rect destination, Rect clip,
	const ImagePatchOptions &options, f32 viewportHeight, f32 scale, Color color, u32 token) {
	renderer.PushClip({clip.x, viewportHeight - clip.y - clip.h, clip.w, clip.h});
	const Rect current = renderer.CurrentClip();
	const Rect visible{current.x, viewportHeight - current.y - current.h, current.w, current.h};
	for (const auto &patch : BuildImagePatches(destination, texture.Size(), texture.flipVertically,
			 options, scale, visible)) {
		renderer.DrawQuad(patch.rect.x, viewportHeight - patch.rect.y - patch.rect.h, patch.rect.w,
			patch.rect.h, patch.uv, texture.Handle(), color, 0, token);
	}
	renderer.PopClip();
}

} // namespace

void DrawUIImage(Renderer2D &renderer, ResourceRegistry &resources, const UIImage &image, Rect rect,
	f32 viewportHeight, f32 scale, Color color, u32 token) {
	const auto texture = resources.Get<ImageTexture>(image.texture);
	if (!texture || !texture->IsLoaded()) {
		return;
	}
	const bool ninePatch = image.mode == UIImageMode::NineSlice;
	const ImagePatchOptions options{
		.regionPosition = image.regionPosition,
		.regionSize = image.regionSize,
		.paddingLeft = image.borderMin.x,
		.paddingRight = image.borderMax.x,
		.paddingTop = image.borderMin.y,
		.paddingBottom = image.borderMax.y,
		.ninePatch = ninePatch,
		.tileCenter = image.mode == UIImageMode::Tile || (ninePatch && image.tileCenter),
		.tileEdges = image.tileEdges,
	};
	Rect destination = rect;
	if (image.mode == UIImageMode::Native) {
		const Vector2 dimensions = texture->Size();
		const Vector2 remaining{dimensions.x -
									std::clamp(image.regionPosition.x, 0.f, dimensions.x),
			dimensions.y - std::clamp(image.regionPosition.y, 0.f, dimensions.y)};
		destination.w =
			std::min(image.regionSize.x > 0 ? image.regionSize.x : remaining.x, remaining.x) *
			scale;
		destination.h =
			std::min(image.regionSize.y > 0 ? image.regionSize.y : remaining.y, remaining.y) *
			scale;
	}
	drawPatches(renderer, *texture, destination, rect, options, viewportHeight, scale, color,
		token);
}

void DrawUINinePatchRect(Renderer2D &renderer, ResourceRegistry &resources,
	const UINinePatchRect &image, Rect rect, f32 viewportHeight, f32 scale, Color color,
	u32 token) {
	const auto texture = resources.Get<ImageTexture>(image.texture);
	if (!texture || !texture->IsLoaded()) {
		return;
	}
	const ImagePatchOptions options{
		.paddingLeft = image.paddingLeft,
		.paddingRight = image.paddingRight,
		.paddingTop = image.paddingTop,
		.paddingBottom = image.paddingBottom,
		.ninePatch = true,
		.tileCenter = image.tileCenter,
		.tileEdges = image.tileEdges,
		.drawCenter = image.drawCenter,
	};
	drawPatches(renderer, *texture, rect, rect, options, viewportHeight, scale, color, token);
}

} // namespace sowa
