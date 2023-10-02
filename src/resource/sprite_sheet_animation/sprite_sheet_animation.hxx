#ifndef SW_SPRITE_SHEET_ANIMATION_HXX
#define SW_SPRITE_SHEET_ANIMATION_HXX
#pragma once

#include <stdint.h>
#include <string>
#include <vector>

#include "core/error/error.hxx"
#include "resource/image_texture/image_texture.hxx"

struct AnimationTexture {
	Texture texture;
	uint32_t col;
	uint32_t row;
};

struct AnimationTile {
	uint32_t texture_index;
	uint32_t col_index;
	uint32_t row_index;
};

struct Animation {
	std::string name;
	std::vector<AnimationTile> tiles;
};

class SpriteSheetAnimation {
  public:
	SpriteSheetAnimation() = default;
	~SpriteSheetAnimation();

	ErrorCode Load(const char *path);

	inline const std::vector<AnimationTexture> &Textures() { return m_textures; }
	inline const std::vector<Animation> &Animations() { return m_animations; }

  private:
	std::vector<AnimationTexture> m_textures;
	std::vector<Animation> m_animations;
};

#endif // SW_SPRITE_SHEET_ANIMATION_HXX