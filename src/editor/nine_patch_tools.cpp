#include "editor/nine_patch_tools.hpp"
#include <algorithm>
#include <cmath>

namespace sowa::editor {

NinePatchRect ResizeNinePatchRect(const NinePatchRect &before, Vector2 delta, Vector2 corner) {
	NinePatchRect result = before;
	result.size = {
		std::max(1.f, before.size.x + delta.x * (corner.x * 2 - 1)),
		std::max(1.f, before.size.y + delta.y * (corner.y * 2 - 1)),
	};
	const Vector2 change = result.size - before.size;
	const f32 pivot = before.centered ? .5f : 0.f;
	result.offset += change * (corner + Vector2(pivot - 1));
	return result;
}

f32 NinePatchPaddingAt(std::string_view field, Vector2 point, Vector2 textureSize, f32 opposite) {
	const bool horizontal = field == "paddingLeft" || field == "paddingRight";
	const bool trailing = field == "paddingRight" || field == "paddingBottom";
	const f32 extent = horizontal ? textureSize.x : textureSize.y;
	const f32 position = horizontal ? point.x : point.y;
	const f32 value = trailing ? extent - position : position;
	return std::clamp(std::round(value), 0.f, std::max(0.f, extent - std::max(0.f, opposite)));
}

} // namespace sowa::editor
