#include "menu_bar_item.hpp"

#include "core/node_db.hpp"
#include "core/application.hpp"
#include "scene/ui/ui_node.hpp"

#include "gfx/graphics.hpp"

namespace sowa {
MenuBarItem::MenuBarItem() {
	_NodeType = "MenuBarItem";
	m_type = Typename();
}

void MenuBarItem::Bind() {
    NodeFactory factory;
	factory.construct = []() -> Node * {
		Node *node = Allocator<MenuBarItem>::Get().allocate(1);
		new (node) MenuBarItem;

		return node;
	};

	factory.destruct = [](Node *node) {
		Allocator<MenuBarItem>::Get().deallocate(reinterpret_cast<MenuBarItem *>(node), 1);
	};

	NodeDB::Instance().RegisterNodeType("MenuBarItem", "UINode", factory);
}

void MenuBarItem::UpdateLogic() {
	m_hovered = Application::get_singleton().HoveringNode() == ID();
}

void MenuBarItem::UpdateDraw() {
    if(nullptr != GetParent() && GetParent()->IsValid() && GetParent()->IsInstanceOf("UINode")) {
        UINode* parent = dynamic_cast<UINode*>(GetParent());
        if(nullptr != parent) {
            vec2 cursor = GetCursorPos();
            parent->Advance(0.f, Size().y);
			parent->ResetX();

			glm::vec2 points[4] = {
				{ 0.0f,  1.0f },
				{ 0.0f,  0.0f },
				{ 1.0f,  0.0f },
				{ 1.0f,  1.0f }
			};
			glm::vec2 uvs[4] = {
				{0.f, 1.f},
				{0.f, 0.f},
				{1.f, 0.f},
				{1.f, 1.f}
			};

			gfx::BatchVertex vertices[4];
			for(int i=0; i<4; i++) {
				points[i].x *= Size().x;
				points[i].y *= Size().y;
				points[i].x += cursor.x;
				points[i].y += cursor.y;

				vertices[i].x = points[i].x;
				vertices[i].y = points[i].y;
				vertices[i].z = 0.f;
				vertices[i].r = m_hovered ? m_hoveredBackgroundColor.r : m_backgroundColor.r;
				vertices[i].g = m_hovered ? m_hoveredBackgroundColor.g : m_backgroundColor.g;
				vertices[i].b = m_hovered ? m_hoveredBackgroundColor.b : m_backgroundColor.b;
				vertices[i].a = m_hovered ? m_hoveredBackgroundColor.a : m_backgroundColor.a;
				vertices[i].uvX = uvs[i].x;
				vertices[i].uvY = uvs[i].y;
				vertices[i].textureId = 0.f;
				vertices[i].id = static_cast<float>(ID());
			}
			Graphics().BatchRendererUI().PushQuad(vertices);
        }
    }
}
} // namespace sowa
