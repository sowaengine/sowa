#ifndef SW_TEXT_2D_HXX
#define SW_TEXT_2D_HXX
#pragma once

#include "scene/nodes/2d/node_2d.hxx"
#include <string>

class Text2D : public Node2D {
  public:
	virtual ~Text2D() = default;

	void _update() override;

	inline float &color_red() { return m_color_red; }
	inline float &color_green() { return m_color_green; }
	inline float &color_blue() { return m_color_blue; }
	inline float &color_alpha() { return m_color_alpha; }

	inline std::string &text() { return m_text; }

  private:
	std::string m_text = "";
	float m_color_red = 1.f;
	float m_color_green = 1.f;
	float m_color_blue = 1.f;
	float m_color_alpha = 1.f;
};

#endif // SW_TEXT_2D_HXX