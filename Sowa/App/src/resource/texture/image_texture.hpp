#ifndef _E_IMAGE_TEXTURE_HPP__
#define _E_IMAGE_TEXTURE_HPP__

#pragma once

#include "stlpch.hpp"

#include "Core/GL/nm_Texture.hpp"

#include "../resource.hpp"
#include "sowa.hpp"
#include "object_type.hpp"
#include "core/file_buffer.hpp"

namespace sowa {

class ImageTexture : public BaseResource, public object_type {
  public:
	ImageTexture();
	~ImageTexture();

	static std::string Typename() { return "sowa::ImageTexture"; }

	inline int Width() { return _texture.GetWidth(); }
	inline int Height() { return _texture.GetHeight(); }
	inline int Channels() { return _texture.GetChannels(); }
	inline unsigned char *Pixels() { return _texture.GetPixels(); }

	size_t TextureID() { return _texture.ID(); }

	static FileBuffer SaveImpl(object_type *);
	static bool LoadImpl(object_type* out, const FileBuffer& doc);

  private:
	template <typename>
	friend class ResourceManager;
	template <typename>
	friend class ResourceLoaderImpl;
	friend class Renderer;

	nmGfx::Texture _texture{};
};

} // namespace sowa
#endif