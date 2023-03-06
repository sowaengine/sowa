#ifndef _E_NINEPATCH_TEXTURE_HPP__
#define _E_NINEPATCH_TEXTURE_HPP__

#pragma once

#include "stlpch.hpp"

#include "Core/GL/nm_Texture.hpp"

#include "../resource.hpp"
#include "image_texture.hpp"
#include "sowa.hpp"

namespace Sowa {

class NinePatchTexture : public ImageTexture {
  public:
	NinePatchTexture();
	~NinePatchTexture();

	inline float &Top() { return _Top; }
	inline float &Left() { return _Left; }
	inline float &Bottom() { return _Bottom; }
	inline float &Right() { return _Right; }

  private:
	template <typename>
	friend class ResourceManager;
	template <typename>
	friend class ResourceLoaderImpl;
	friend class Renderer;

	float _Top{0.f};
	float _Left{0.f};
	float _Bottom{0.f};
	float _Right{0.f};
};

} // namespace Sowa
#endif