#pragma once

#include <map>
#include <string>
#include <vector>

#include "core/types.hpp"
#include "math/rect.hpp"
#include "math/vector2.hpp"
#include "resource/resource.hpp"
#include "resource/resource_id.hpp"

namespace sowa {

struct SpriteAnimation {
	ResourceID texture;
	i32 columns = 1;
	i32 rows = 1;

	// Outer border and gaps between cells, in pixels on each axis.
	i32 paddingX = 0;
	i32 paddingY = 0;
	i32 spacingX = 0;
	i32 spacingY = 0;

	// Row-major cell indices from the top-left, in playback order. Repeats are allowed.
	std::vector<i32> frames;
	f32 framesPerSecond = 12.f;
	bool loop = true;

	i32 FrameCount() const {
		return static_cast<i32>(frames.size());
	}

	f32 Duration() const;
	bool IsValid() const;

	// Pixel rectangle from the top-left, or an empty rectangle for invalid cells.
	Rect CellRect(i32 cell, Vector2 textureSize) const;
	Rect FrameUV(i32 index, Vector2 textureSize = {1, 1}, bool flipVertically = true) const;
};

// Shared animation data. Per-entity playback lives in AnimatedSprite.
class SpriteSheetAnimation : public Resource {
  public:
	void Serialize(Archive &archive) override;
	SpriteAnimation *Find(const std::string &name);
	const SpriteAnimation *Find(const std::string &name) const;

	// Set adds or replaces an animation; empty names are refused.
	bool Set(const std::string &name, const SpriteAnimation &animation);
	bool Remove(const std::string &name);
	bool Has(const std::string &name) const;
	void Clear();

	std::size_t Count() const {
		return _animations.size();
	}

	const std::map<std::string, SpriteAnimation> &Animations() const {
		return _animations;
	}

	std::vector<std::string> Names() const;

  private:
	std::map<std::string, SpriteAnimation> _animations;
};

} // namespace sowa
