#ifndef _E_TEXT2D_HPP__
#define _E_TEXT2D_HPP__

#pragma once
#include <glm/glm.hpp>
#include <string>

namespace Ease::Component {
class Text2D {
  public:
	Text2D();
	Text2D(const std::string &text) : m_Text(text) {}
	~Text2D();

	std::string &Text() { return m_Text; }
	glm::vec4 &Color() { return m_Color; }
	float &FontSize() { return m_FontSize; }
	bool &Visible() { return m_Visible; }

  private:
	std::string m_Text;
	glm::vec4 m_Color{255.f, 255.f, 255.f, 255.f};
	float m_FontSize = 64.f;
	bool m_Visible = true;
};
} // namespace Ease::Component

#endif