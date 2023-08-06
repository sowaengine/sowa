#ifndef SW_TEXT_HXX
#define SW_TEXT_HXX
#pragma once

#include "glm/glm.hpp"
#include <string>

#include "node.hxx"

class Text : public Node {
  public:
	virtual ~Text() = default;

	void Update() override;

	inline std::string &GetText() { return m_text; }
	inline glm::vec2 &Position() { return m_position; }

  private:
	std::string m_text = "";
	glm::vec2 m_position = glm::vec2(0.f, 0.f);
};

#endif // SW_TEXT_HXX