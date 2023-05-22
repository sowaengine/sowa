#ifndef SW_UI_PANEL_HPP
#define SW_UI_PANEL_HPP

#include "scene/node.hpp"
#include "math/math.hpp"

namespace sowa {
class UINode : public Node {
  public:
	UINode();

	static std::string Typename() { return "sowa::UINode"; }
	static void Bind();

	void PreUpdate() override;

	void Advance(float x, float y);
	void ResetX();
	void ResetY();

	inline vec2& Position() { return m_position; }
	inline vec2& Size() { return m_size; }

	vec2 GetCursorPos();

  protected:
	friend class Application;

	vec2 m_position = vec2(0.f, 0.f);
	vec2 m_size = vec2(128.f, 128.f);

	float m_cursorX = 0.f;
	float m_cursorY = 0.f;
};
} // namespace sowa

#endif // SW_UI_PANEL_HPP