#ifndef SW_MENUBAR_HPP_
#define SW_MENUBAR_HPP_

#include "scene/ui/ui_node.hpp"

#include "glm/glm.hpp"

namespace sowa {
class MenuBar : public UINode {
  public:
	MenuBar();

	static std::string Typename() { return "sowa::MenuBar"; }
	static void Bind();

    void UpdateDraw() override;

  protected:
	friend class Application;

	glm::vec4 m_backgroundColor = glm::vec4(0.2f, 0.2f, 0.2f, 1.f);
};
} // namespace sowa

#endif // SW_MENUBAR_HPP_