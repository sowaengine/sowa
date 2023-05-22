#ifndef SW_MENUBAR_ITEM_HPP_
#define SW_MENUBAR_ITEM_HPP_

#include "scene/ui/ui_node.hpp"

namespace sowa {
class MenuBarItem : public UINode {
  public:
	MenuBarItem();

	static std::string Typename() { return "sowa::MenuBarItem"; }
	static void Bind();

    void UpdateDraw() override;

  protected:
	friend class Application;
};
} // namespace sowa

#endif // SW_MENUBAR_ITEM_HPP_