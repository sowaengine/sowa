#pragma once

#include "math/rect.hpp"
#include "math/vector2.hpp"
#include <optional>
#include <vector>

namespace sowa {

struct ImagePatchOptions {
	Vector2 regionPosition{0, 0};
	Vector2 regionSize{0, 0};
	f32 paddingLeft = 0;
	f32 paddingRight = 0;
	f32 paddingTop = 0;
	f32 paddingBottom = 0;
	bool ninePatch = false;
	bool tileCenter = false;
	bool tileEdges = false;
	bool drawCenter = true;
};

struct ImagePatch {
	Rect rect;
	Rect uv;
};

// Destination and source use top-left coordinates. UVs address a Y-up quad.
// Clipping limits tile generation; the renderer still clips boundary quads.
std::vector<ImagePatch> BuildImagePatches(Rect destination, Vector2 textureSize,
	bool flipVertically, const ImagePatchOptions &options, f32 scale = 1,
	std::optional<Rect> clip = {});

} // namespace sowa
