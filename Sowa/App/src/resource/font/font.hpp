#ifndef _E_FONT_HPP__
#define _E_FONT_HPP__

#pragma once

#include "stlpch.hpp"

#include "Core/GL/nm_Font.hpp"

#include "../resource.hpp"
#include "sowa.hpp"

namespace Sowa {

class Font : public BaseResource {
  public:
	Font();
	~Font();


  private:
	template <typename>
	friend class ResourceManager;
	template <typename>
	friend class ResourceLoaderImpl;
	friend class Renderer;

	nmGfx::Font _Font{};
};

} // namespace Sowa
#endif