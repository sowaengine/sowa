#include "ui_tree.hxx"

#include <iostream>

UITree::UITree() {
	lay_init_context(&m_ctx);
	lay_reserve_items_capacity(&m_ctx, 1024);
}

UITree::~UITree() {
	lay_destroy_context(&m_ctx);
}

static float CalculateUIItemSize(UISize &s, float elemSize) {
	if (s.Scale() == "%") {
		return elemSize * (s.Number() * 0.01);
	} else {
		return s.Number();
	}
}

void UITree::SetRoot(TreeNode<UIContainer> &root) {
	m_rootID = root.ID();
}

void UITree::Calculate() {
	TreeNode<UIContainer> *root = m_tree.FindNodeByID(m_rootID);
	if (nullptr == root) {
		return;
	}

	if (m_calculateCount != 0) {
		lay_reset_context(&m_ctx);
	}

	const auto processUINode = [this](const auto &self, TreeNode<UIContainer> *container, TreeNode<UIContainer> *parent) -> void {
		if (nullptr == container || !container->Node().active) {
			return;
		}

		container->Node().m_uitree_item = lay_item(&m_ctx);
		if (nullptr != parent && parent->Node().m_uitree_item != LAY_INVALID_ID) {
			lay_insert(&m_ctx, parent->Node().m_uitree_item, container->Node().m_uitree_item);
		}

		uint32_t containFlags = 0;
		uint32_t behaveFlags = 0;

		if (container->Node().flexDirection == FlexDirection::Column) {
			containFlags |= LAY_COLUMN;
		} else if (container->Node().flexDirection == FlexDirection::Row) {
			containFlags |= LAY_ROW;
		}

		if (container->Node().layoutModel == LayoutModel::Free) {
			containFlags |= LAY_LAYOUT;
		} else if (container->Node().layoutModel == LayoutModel::Flex) {
			containFlags |= LAY_FLEX;
		}

		if (container->Node().wrap == Wrap::NoWrap) {
			containFlags |= LAY_NOWRAP;
		} else if (container->Node().wrap == Wrap::Wrap) {
			containFlags |= LAY_WRAP;
		}

		if (container->Node().justifyContent == JustifyContent::Start) {
			containFlags |= LAY_START;
		} else if (container->Node().justifyContent == JustifyContent::Middle) {
			containFlags |= LAY_MIDDLE;
		} else if (container->Node().justifyContent == JustifyContent::End) {
			containFlags |= LAY_END;
		} else if (container->Node().justifyContent == JustifyContent::Justify) {
			containFlags |= LAY_JUSTIFY;
		}

		if (container->Node().anchor == Anchor::Left) {
			behaveFlags |= LAY_LEFT;
		} else if (container->Node().anchor == Anchor::Top) {
			behaveFlags |= LAY_TOP;
		} else if (container->Node().anchor == Anchor::Right) {
			behaveFlags |= LAY_RIGHT;
		} else if (container->Node().anchor == Anchor::Bottom) {
			behaveFlags |= LAY_BOTTOM;
		} else if (container->Node().anchor == Anchor::HFill) {
			behaveFlags |= LAY_HFILL;
		} else if (container->Node().anchor == Anchor::VFill) {
			behaveFlags |= LAY_VFILL;
		} else if (container->Node().anchor == Anchor::HCenter) {
			behaveFlags |= LAY_HCENTER;
		} else if (container->Node().anchor == Anchor::VCenter) {
			behaveFlags |= LAY_VCENTER;
		} else if (container->Node().anchor == Anchor::Center) {
			behaveFlags |= LAY_CENTER;
		} else if (container->Node().anchor == Anchor::Fill) {
			behaveFlags |= LAY_FILL;
		}

		if (nullptr != parent) {
			lay_set_size_xy(&m_ctx, container->Node().m_uitree_item, CalculateUIItemSize(container->Node().width, parent->Node().m_uitree_w), CalculateUIItemSize(container->Node().height, parent->Node().m_uitree_h));
		} else {
			lay_set_size_xy(&m_ctx, container->Node().m_uitree_item, container->Node().width.Number(), container->Node().height.Number());
		}

		lay_set_margins_ltrb(&m_ctx, container->Node().m_uitree_item, container->Node().margin.left, container->Node().margin.top, container->Node().margin.right, container->Node().margin.bottom);
		if (containFlags != 0) {
			lay_set_contain(&m_ctx, container->Node().m_uitree_item, containFlags);
		}
		if (behaveFlags != 0) {
			lay_set_behave(&m_ctx, container->Node().m_uitree_item, behaveFlags);
		}

		for (TreeNode<UIContainer> *child : container->GetChildren()) {
			self(self, child, container);
		}
	};

	processUINode(processUINode, root, nullptr);

	lay_run_item(&m_ctx, root->Node().m_uitree_item);
	// lay_run_context(&m_ctx);
	m_calculateCount++;

	auto positionUINode = [&root, this](const auto &self, TreeNode<UIContainer> *container) {
		if (nullptr == container || !container->Node().active) {
			return;
		}

		lay_scalar x, y, w, h;
		lay_get_rect_xywh(&m_ctx, container->Node().m_uitree_item, &x, &y, &w, &h);
		x += container->Node().padding.left;
		y += container->Node().padding.bottom;

		w -= container->Node().padding.left;
		w -= container->Node().padding.right;
		h -= container->Node().padding.bottom;
		h -= container->Node().padding.top;

		container->Node().m_uitree_x = static_cast<float>(x);
		container->Node().m_uitree_y = static_cast<float>(y);
		container->Node().m_uitree_w = static_cast<float>(w);
		container->Node().m_uitree_h = static_cast<float>(h);
		ModelBuilder::UIQuad(
			container->Node().m_model,
			(x / root->Node().width.Number()) * 2,
			(y / root->Node().height.Number()) * 2,
			(w / root->Node().width.Number()) * 2,
			(h / root->Node().height.Number()) * 2,
			container->Node().backgroundColor.r,
			container->Node().backgroundColor.g,
			container->Node().backgroundColor.b,
			container->Node().backgroundColor.a,
			container->ID());

		for (TreeNode<UIContainer> *child : container->GetChildren()) {
			self(self, child);
		}
	};
	positionUINode(positionUINode, root);
}

void UITree::DrawLayout() {
	TreeNode<UIContainer> *root = m_tree.FindNodeByID(m_rootID);
	if (nullptr != root) {
		drawContainer(*root);
	}
}

void UITree::drawContainer(TreeNode<UIContainer> &container) {

	if (container.Node().active) {
		if (container.Node().visible) {
			container.Node().m_model.Draw();
		}

		for (TreeNode<UIContainer> *child : container.GetChildren()) {
			if (nullptr != child)
				drawContainer(*child);
		}
	}
}