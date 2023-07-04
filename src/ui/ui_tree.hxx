#ifndef SW_UI_TREE
#define SW_UI_TREE
#pragma once

#include "layout.h"
#include "ui_container.hxx"

class UITree {
  public:
	UITree();
	~UITree();

	UIContainer &Root() { return m_root; }

	void Calculate();
	void DrawLayout();

  private:
	UIContainer m_root;
	lay_context m_ctx;

	void drawContainer(UIContainer&);

	uint32_t m_calculateCount = 0;
};

#endif // SW_UI_TREE