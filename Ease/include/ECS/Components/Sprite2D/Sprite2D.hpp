#ifndef _E_SPRITE2D_HPP__
#define _E_SPRITE2D_HPP__

#pragma once
#include <memory>
#include <stdint.h>

namespace Ease {
class ImageTexture;
}

namespace Ease::Component {
class Sprite2D {
  public:
	Sprite2D();
	~Sprite2D();

	std::shared_ptr<Ease::ImageTexture> &Texture() { return _texture; }
	unsigned int &TextureID() { return _textureID; }
	bool &Visible() { return m_Visible; }

  private:
	std::shared_ptr<Ease::ImageTexture> _texture{nullptr};
	unsigned int _textureID{0};
	bool m_Visible = true;
};
} // namespace Ease::Component

#endif