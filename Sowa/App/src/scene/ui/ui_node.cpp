#include "ui_node.hpp"

#include "core/node_db.hpp"

/**
 * MenuBarItem::UpdateDraw:
 *  if parent(menubaritem).selected || parent is menubar {
 *      drawSelf()
 *  }
 * OnInput:
 *  if onMe:
 *      selected = true;
 *      do action (like save scene, open file etc.)
 *  else:
 *   if notOnAnyOfChildren:
 *      selected = false;
 */

namespace sowa {
UINode::UINode() {
	_NodeType = "UINode";
	m_type = Typename();
}

void UINode::Bind() {
	NodeFactory factory;
	factory.construct = []() -> Node * {
		Node *node = Allocator<UINode>::Get().allocate(1);
		new (node) UINode;

		return node;
	};

	factory.destruct = [](Node *node) {
		Allocator<UINode>::Get().deallocate(reinterpret_cast<UINode *>(node), 1);
	};

	NodeDB::Instance().RegisterNodeType("UINode", "Node", factory);

	NodeDB::Instance().RegisterAttribute<float>(
		"UINode", "position.x", [](Node *node) -> float {
		UINode* uinode = dynamic_cast<UINode*>(node);
		if(nullptr != uinode) {
			return uinode->Position().x;
		}
		return 0.f; }, [](Node *node, float pos) {
		UINode* uinode = dynamic_cast<UINode*>(node);
		if(nullptr != uinode) {
			uinode->Position().x = pos;
		} });

	NodeDB::Instance().RegisterAttribute<float>(
		"UINode", "position.y", [](Node *node) -> float {
		UINode* uinode = dynamic_cast<UINode*>(node);
		if(nullptr != uinode) {
			return uinode->Position().y;
		}
		return 0.f; }, [](Node *node, float pos) {
		UINode* uinode = dynamic_cast<UINode*>(node);
		if(nullptr != uinode) {
			uinode->Position().y = pos;
		} });

	NodeDB::Instance().RegisterAttribute<float>(
		"UINode", "size.x", [](Node *node) -> float {
		UINode* uinode = dynamic_cast<UINode*>(node);
		if(nullptr != uinode) {
			return uinode->Size().x;
		}
		return 0.f; }, [](Node *node, float sz) {
		UINode* uinode = dynamic_cast<UINode*>(node);
		if(nullptr != uinode) {
			uinode->Size().x = sz;
		} });

	NodeDB::Instance().RegisterAttribute<float>(
		"UINode", "size.y", [](Node *node) -> float {
		UINode* uinode = dynamic_cast<UINode*>(node);
		if(nullptr != uinode) {
			return uinode->Size().y;
		}
		return 0.f; }, [](Node *node, float sz) {
		UINode* uinode = dynamic_cast<UINode*>(node);
		if(nullptr != uinode) {
			uinode->Size().y = sz;
		} });
}

void UINode::PreUpdate() {
	m_cursorX = 0.f;
	m_cursorY = 0.f;
}

void UINode::Advance(float x, float y) {
	m_cursorX += x;
	m_cursorY += y;
}

void UINode::ResetX() {
	m_cursorX = 0.f;
}

void UINode::ResetY() {
	m_cursorY = 0.f;
}

vec2 UINode::GetCursorPos() {
	if (nullptr != GetParent() && GetParent()->IsValid() && GetParent()->IsInstanceOf("UINode")) {
		UINode *parent = dynamic_cast<UINode *>(GetParent());
		if (nullptr != parent) {
			vec2 pos = parent->GetCursorPos();
			pos.x += m_cursorX;
			pos.y += m_cursorY;

			return pos;
		}
	}
	return vec2(Position().x + m_cursorX, Position().y + m_cursorY);
}

} // namespace sowa
