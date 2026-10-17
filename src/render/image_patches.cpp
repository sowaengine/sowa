#include "render/image_patches.hpp"
#include <algorithm>
#include <cmath>

namespace sowa {

std::vector<ImagePatch> BuildImagePatches(Rect destination, Vector2 textureSize,
	bool flipVertically, const ImagePatchOptions &options, f32 scale, std::optional<Rect> clip) {
	std::vector<ImagePatch> patches;
	if (destination.w <= 0 || destination.h <= 0 || textureSize.x <= 0 || textureSize.y <= 0 ||
		scale <= 0) {
		return patches;
	}
	const Vector2 origin{std::clamp(options.regionPosition.x, 0.f, textureSize.x),
		std::clamp(options.regionPosition.y, 0.f, textureSize.y)};
	const Vector2 size{
		std::min(options.regionSize.x > 0 ? options.regionSize.x : textureSize.x - origin.x,
			textureSize.x - origin.x),
		std::min(options.regionSize.y > 0 ? options.regionSize.y : textureSize.y - origin.y,
			textureSize.y - origin.y)};
	if (size.x <= 0 || size.y <= 0) {
		return patches;
	}
	const Rect visible = clip.value_or(destination);
	auto draw = [&](Rect area, Rect source, bool tileX, bool tileY) {
		if (area.w <= 0 || area.h <= 0 || source.w <= 0 || source.h <= 0) {
			return;
		}
		const f32 tileWidth = tileX ? std::max(1.f, source.w * scale) : area.w;
		const f32 tileHeight = tileY ? std::max(1.f, source.h * scale) : area.h;
		const f32 startX = std::floor(std::max(0.f, visible.x - area.x) / tileWidth) * tileWidth;
		const f32 startY = std::floor(std::max(0.f, visible.y - area.y) / tileHeight) * tileHeight;
		const f32 endX = std::min(area.w, visible.Right() - area.x);
		const f32 endY = std::min(area.h, visible.Top() - area.y);
		for (f32 y = startY; y < endY; y += tileHeight) {
			for (f32 x = startX; x < endX; x += tileWidth) {
				const f32 width = std::min(tileWidth, area.w - x);
				const f32 height = std::min(tileHeight, area.h - y);
				const f32 u = (origin.x + source.x) / textureSize.x;
				const f32 v = (origin.y + source.y) / textureSize.y;
				const f32 uvWidth = source.w * width / tileWidth / textureSize.x;
				const f32 uvHeight = source.h * height / tileHeight / textureSize.y;
				const Rect uv = flipVertically ? Rect(u, 1 - v - uvHeight, uvWidth, uvHeight)
											   : Rect(u, v + uvHeight, uvWidth, -uvHeight);
				patches.push_back({{area.x + x, area.y + y, width, height}, uv});
			}
		}
	};
	if (!options.ninePatch) {
		draw(destination, {0, 0, size.x, size.y}, options.tileCenter, options.tileCenter);
		return patches;
	}

	const f32 left = std::clamp(options.paddingLeft, 0.f, size.x);
	const f32 right = std::clamp(options.paddingRight, 0.f, size.x - left);
	const f32 top = std::clamp(options.paddingTop, 0.f, size.y);
	const f32 bottom = std::clamp(options.paddingBottom, 0.f, size.y - top);
	const f32 scaleX = left + right > 0 ? std::min(scale, destination.w / (left + right)) : scale;
	const f32 scaleY = top + bottom > 0 ? std::min(scale, destination.h / (top + bottom)) : scale;
	const f32 sourceX[] = {0, left, size.x - right, size.x};
	const f32 sourceY[] = {0, top, size.y - bottom, size.y};
	const f32 destX[] = {destination.x, destination.x + left * scaleX,
		destination.Right() - right * scaleX, destination.Right()};
	const f32 destY[] = {destination.y, destination.y + top * scaleY,
		destination.Top() - bottom * scaleY, destination.Top()};
	for (int y = 0; y < 3; ++y) {
		for (int x = 0; x < 3; ++x) {
			const bool center = x == 1 && y == 1;
			if (center && !options.drawCenter) {
				continue;
			}
			const bool tile = center ? options.tileCenter : options.tileEdges;
			draw({destX[x], destY[y], destX[x + 1] - destX[x], destY[y + 1] - destY[y]},
				{sourceX[x], sourceY[y], sourceX[x + 1] - sourceX[x], sourceY[y + 1] - sourceY[y]},
				x == 1 && tile, y == 1 && tile);
		}
	}
	return patches;
}

} // namespace sowa
