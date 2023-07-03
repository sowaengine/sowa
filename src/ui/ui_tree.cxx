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

void UITree::Calculate() {
	if (m_calculateCount != 0) {
		lay_reset_context(&m_ctx);
	}

	const auto processUINode = [this](const auto &self, UIContainer *container, UIContainer *parent) -> void {
		if (nullptr == container || !container->active) {
			return;
		}

		container->m_uitree_item = lay_item(&m_ctx);
		if (nullptr != parent && parent->m_uitree_item != LAY_INVALID_ID) {
			lay_insert(&m_ctx, parent->m_uitree_item, container->m_uitree_item);
		}

		uint32_t containFlags = 0;
		uint32_t behaveFlags = 0;

		if (container->flexDirection == FlexDirection::Column) {
			containFlags |= LAY_COLUMN;
		} else if (container->flexDirection == FlexDirection::Row) {
			containFlags |= LAY_ROW;
		}

		if (container->layoutModel == LayoutModel::Free) {
			containFlags |= LAY_LAYOUT;
		} else if (container->layoutModel == LayoutModel::Flex) {
			containFlags |= LAY_FLEX;
		}

		if (container->wrap == Wrap::NoWrap) {
			containFlags |= LAY_NOWRAP;
		} else if (container->wrap == Wrap::Wrap) {
			containFlags |= LAY_WRAP;
		}

		if (container->justifyContent == JustifyContent::Start) {
			containFlags |= LAY_START;
		} else if (container->justifyContent == JustifyContent::Middle) {
			containFlags |= LAY_MIDDLE;
		} else if (container->justifyContent == JustifyContent::End) {
			containFlags |= LAY_END;
		} else if (container->justifyContent == JustifyContent::Justify) {
			containFlags |= LAY_JUSTIFY;
		}

		if (container->anchor == Anchor::Left) {
			behaveFlags |= LAY_LEFT;
		} else if (container->anchor == Anchor::Top) {
			behaveFlags |= LAY_TOP;
		} else if (container->anchor == Anchor::Right) {
			behaveFlags |= LAY_RIGHT;
		} else if (container->anchor == Anchor::Bottom) {
			behaveFlags |= LAY_BOTTOM;
		} else if (container->anchor == Anchor::HFill) {
			behaveFlags |= LAY_HFILL;
		} else if (container->anchor == Anchor::VFill) {
			behaveFlags |= LAY_VFILL;
		} else if (container->anchor == Anchor::HCenter) {
			behaveFlags |= LAY_HCENTER;
		} else if (container->anchor == Anchor::VCenter) {
			behaveFlags |= LAY_VCENTER;
		} else if (container->anchor == Anchor::Center) {
			behaveFlags |= LAY_CENTER;
		} else if (container->anchor == Anchor::Fill) {
			behaveFlags |= LAY_FILL;
		}

		if (nullptr != parent) {
			lay_set_size_xy(&m_ctx, container->m_uitree_item, CalculateUIItemSize(container->width, parent->m_uitree_w), CalculateUIItemSize(container->height, parent->m_uitree_h));
		} else {
			lay_set_size_xy(&m_ctx, container->m_uitree_item, container->width.Number(), container->height.Number());
		}

		lay_set_margins_ltrb(&m_ctx, container->m_uitree_item, container->margin.left, container->margin.top, container->margin.right, container->margin.bottom);
		if (containFlags != 0) {
			lay_set_contain(&m_ctx, container->m_uitree_item, containFlags);
		}
		if (behaveFlags != 0) {
			lay_set_behave(&m_ctx, container->m_uitree_item, behaveFlags);
		}

		for (UIContainer &child : container->Children()) {
			self(self, &child, container);
		}
	};
	processUINode(processUINode, &m_root, nullptr);

	lay_run_item(&m_ctx, m_root.m_uitree_item);
	// lay_run_context(&m_ctx);
	m_calculateCount++;

	auto positionUINode = [this](const auto &self, UIContainer *container) {
		if (nullptr == container) {
			return;
		}

		lay_scalar x, y, w, h;
		lay_get_rect_xywh(&m_ctx, container->m_uitree_item, &x, &y, &w, &h);
		x += container->padding.left;
		y += container->padding.bottom;

		w -= container->padding.left;
		w -= container->padding.right;
		h -= container->padding.bottom;
		h -= container->padding.top;

		container->m_uitree_x = static_cast<float>(x);
		container->m_uitree_y = static_cast<float>(y);
		container->m_uitree_w = static_cast<float>(w);
		container->m_uitree_h = static_cast<float>(h);
		ModelBuilder::UIQuad(
			container->m_model,
			(x / m_root.width.Number()) * 2,
			(y / m_root.height.Number()) * 2,
			(w / m_root.width.Number()) * 2,
			(h / m_root.height.Number()) * 2,
			container->backgroundColor.r,
			container->backgroundColor.g,
			container->backgroundColor.b,
			container->backgroundColor.a,
			container->id);

		for (UIContainer &child : container->Children()) {
			self(self, &child);
		}
	};
	positionUINode(positionUINode, &m_root);
}