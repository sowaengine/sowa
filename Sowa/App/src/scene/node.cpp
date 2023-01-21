#include "node.hpp"
#include "scene.hpp"

namespace Sowa {
Node::Node() : _Name("Node") {}
Node::Node(const std::string &name) : _Name(name) {}
Node::~Node() {
	if (IsValid()) {
	}
}

void Node::AddChild(Node *node) {
	if (node == nullptr)
		return;

	_Children.push_back(node);
	node->SetParent(this);
}
Node *Node::GetChild(const std::string &name) {
	for (Node *node : _Children) {
		if (name == node->Name()) {
			return node;
		}
	}
	return nullptr;
}
void Node::RemoveChild(const std::string &name) {
	for (Node *node : _Children) {
		if (name == node->Name()) {
			return RemoveNode(node);
		}
	}
}

void Node::RemoveNode(Node *node) {
	for (size_t i = 0; i < _Children.size(); i++) {
		Node *child = _Children[i];
		if (child == node) {
			if (node->GetParent() != nullptr)
				node->SetParent(nullptr);
			_Children.erase(_Children.begin() + i);
			return;
		}
	}
}

void Node::SetParent(Node *node) {
	_Parent = node;
	if (_Parent != nullptr && _Parent != node)
		node->RemoveNode(this);
}

} // namespace Sowa
