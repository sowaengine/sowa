#ifndef SW_UI_TREE
#define SW_UI_TREE
#pragma once

#include "data/tree.hxx"
#include "layout.h"
#include "ui_container.hxx"

class UITree {
  public:
	UITree();
	~UITree();

	void SetRoot(TreeNode<UIContainer> &root);
	inline const int RootID() { return m_rootID; }

	Tree<UIContainer> &GetTree() { return m_tree; }

	void Calculate();
	void DrawLayout();

	float GetGlobalX(TreeNode<UIContainer> &node);
	float GetGlobalY(TreeNode<UIContainer> &node);

  private:
	lay_context m_ctx;

	Tree<UIContainer> m_tree;
	int m_rootID = 0;

	void drawContainer(TreeNode<UIContainer> &);

	uint32_t m_calculateCount = 0;
};

#endif // SW_UI_TREE