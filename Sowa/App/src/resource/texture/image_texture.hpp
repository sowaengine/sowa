#ifndef _E_IMAGE_TEXTURE_HPP__
#define _E_IMAGE_TEXTURE_HPP__

#pragma once

#include "stlpch.hpp"

#include "gfx/gl/texture_gl.hpp"

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

	inline int Width() { return m_texture.Width(); }
	inline int Height() { return m_texture.Height(); }
	inline int Channels() { return m_texture.Channels(); }
	inline unsigned char *Pixels() { return m_texture.Pixels(); }

	size_t TextureID() { return m_texture.ID(); }

	static FileBuffer SaveImpl(object_type *);
	static bool LoadImpl(object_type* out, const FileBuffer& doc);

  private:
	template <typename>
	friend class ResourceManager;
	template <typename>
	friend class ResourceLoaderImpl;
	friend class Renderer;

	gfx::GLTexture m_texture{};
};

} // namespace sowa
#endif