#ifndef SW_SPRITE_2D_HXX
#define SW_SPRITE_2D_HXX
#pragma once

#include "node_2d.hxx"

#include "core/rendering/texture.hxx"
#include <memory>

class Sprite2D : public Node2D {
  public:
	void Update() override;

	inline std::shared_ptr<Texture> &GetTexture() { return m_texture; }

  protected:
	std::shared_ptr<Texture> m_texture = nullptr;
};

#endif // SW_SPRITE_2D_HXX