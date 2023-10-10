#ifndef SW_TEXT_HXX
#define SW_TEXT_HXX
#pragma once

#include "glm/glm.hpp"
#include <string>

#include "scene/nodes/node.hxx"

class Text : public Node {
  public:
	virtual ~Text() = default;

	void _update() override;

	inline std::string &text() { return m_text; }
	inline glm::vec2 &position() { return m_position; }

  private:
	std::string m_text = "";
	glm::vec2 m_position = glm::vec2(0.f, 0.f);
};

#endif // SW_TEXT_HXX