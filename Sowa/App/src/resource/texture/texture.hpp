#ifndef _E_TEXTURE_HPP__
#define _E_TEXTURE_HPP__

#pragma once

#include "stlpch.hpp"

#include "Core/GL/nm_Texture.hpp"

#include "../resource.hpp"
#include "sowa.hpp"

namespace Sowa {

class ImageTexture : public BaseResource {
  public:
	ImageTexture();
	~ImageTexture();

	inline int Width() { return _texture.GetWidth(); }
	inline int Height() { return _texture.GetHeight(); }
	inline int Channels() { return _texture.GetChannels(); }
	inline unsigned char *Pixels() { return _texture.GetPixels(); }

	size_t TextureID() { return _texture.ID(); }

  private:
	template <typename>
	friend class ResourceManager;
	template <typename>
	friend class ResourceLoaderImpl;
	friend class Renderer;

	nmGfx::Texture _texture{};
};

} // namespace Sowa
#endif