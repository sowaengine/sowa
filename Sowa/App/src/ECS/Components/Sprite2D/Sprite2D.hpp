#ifndef _E_SPRITE2D_HPP__
#define _E_SPRITE2D_HPP__

#pragma once
#include "Resource/Texture/Texture.hpp"
#include "stlpch.hpp"

namespace Sowa {
class ImageTexture;
}

namespace Sowa::Component {
class Sprite2D {
  public:
	Sprite2D();
	~Sprite2D();

	std::shared_ptr<Sowa::ImageTexture> &Texture() { return _texture; }
	bool &Visible() { return m_Visible; }

  public:
	std::shared_ptr<Sowa::ImageTexture> _texture{nullptr};
	bool m_Visible = true;
};
} // namespace Sowa::Component

#endif