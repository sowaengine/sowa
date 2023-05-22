#include "menu_bar.hpp"

#include "core/node_db.hpp"
#include "scene/ui/ui_node.hpp"

#include "gfx/graphics.hpp"


namespace sowa {
MenuBar::MenuBar() {
	_NodeType = "MenuBar";
	m_type = Typename();
}

void MenuBar::Bind() {
    NodeFactory factory;
	factory.construct = []() -> Node * {
		Node *node = Allocator<MenuBar>::Get().allocate(1);
		new (node) MenuBar;

		return node;
	};

	factory.destruct = [](Node *node) {
		Allocator<MenuBar>::Get().deallocate(reinterpret_cast<MenuBar *>(node), 1);
	};

	NodeDB::Instance().RegisterNodeType("MenuBar", "UINode", factory);
}

void MenuBar::UpdateDraw() {
    if(nullptr != GetParent() && GetParent()->IsValid() && GetParent()->IsInstanceOf("UINode")) {
        UINode* parent = dynamic_cast<UINode*>(GetParent());
        if(nullptr != parent) {
            vec2 cursor = GetCursorPos();
            parent->Advance(0.f, -Size().y);
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
				points[i].y *= -Size().y;
				points[i].x += cursor.x;
				points[i].y += cursor.y;

				vertices[i].x = points[i].x;
				vertices[i].y = points[i].y;
				vertices[i].z = 0.f;
				vertices[i].r = .6f;
				vertices[i].g = .8f;
				vertices[i].b = .6f;
				vertices[i].a = 1.f;
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
