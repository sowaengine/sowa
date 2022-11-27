#ifndef _E_NINEPATCHRECT_HPP__
#define _E_NINEPATCHRECT_HPP__

#pragma once
#include "Resource/Texture/Texture.hpp"
#include <memory>
#include <stdint.h>

namespace Sowa::Component {
class NinePatchRect {
  public:
	NinePatchRect();
	~NinePatchRect();

	std::weak_ptr<Sowa::ImageTexture> &Texture() { return m_Texture; }
	bool &Visible() { return m_Visible; }

	int &Left() { return m_Left; }
	int &Right() { return m_Right; }
	int &Top() { return m_Top; }
	int &Bottom() { return m_Bottom; }

  private:
	std::weak_ptr<Sowa::ImageTexture> m_Texture;
	bool m_Visible = true;

	int m_Left;
	int m_Top;
	int m_Right;
	int m_Bottom;
};
} // namespace Sowa::Component

#endif