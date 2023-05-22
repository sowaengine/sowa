#ifndef SW_MENUBAR_ITEM_HPP_
#define SW_MENUBAR_ITEM_HPP_

#include "scene/ui/ui_node.hpp"
#include "glm/glm.hpp"

namespace sowa {
class MenuBarItem : public UINode {
  public:
	MenuBarItem();

	static std::string Typename() { return "sowa::MenuBarItem"; }
	static void Bind();

	void UpdateLogic() override;
    void UpdateDraw() override;

  protected:
	friend class Application;

	glm::vec4 m_backgroundColor = glm::vec4(0.0f, 0.4f, 0.6f, 1.0f);
	glm::vec4 m_hoveredBackgroundColor = glm::vec4(0.0f, 0.55f, 0.7f, 1.0f);

	bool m_hovered = true;
};
} // namespace sowa

#endif // SW_MENUBAR_ITEM_HPP_