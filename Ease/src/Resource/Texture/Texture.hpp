#ifndef _E_TEXTURE_HPP__
#define _E_TEXTURE_HPP__

#pragma once

#include "../Resource.hpp"
#include "Core/GL/nm_Texture.hpp"
#include <memory>
#include <stdint.h>
#include <string>

typedef uint32_t ResourceID;

namespace Ease {

class ImageTexture : public BaseResource {
  public:
	ImageTexture();
	~ImageTexture();

	const std::string &GetFilepath() { return m_Filepath; }
	void SetFilepath(const std::string &path) { m_Filepath = path; }
	inline int Width() { return _texture.GetWidth(); }
	inline int Height() { return _texture.GetHeight(); }
	inline int Channels() { return _texture.GetChannels(); }

	size_t TextureID() { return _texture.ID(); }

  private:
	template <typename>
	friend class ResourceManager;
	template <typename>
	friend class ResourceLoaderImpl;
	friend class Renderer;

	nmGfx::Texture _texture{};
	std::string m_Filepath{""};
};

} // namespace Ease
#endif