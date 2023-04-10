#include "node.hpp"
#include "scene.hpp"
#include "serialize/serializer.hpp"

namespace Sowa {
Node::Node() : _Name("Node") {
	m_Type = Typename();
}
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

FileBuffer Node::SaveImpl(ObjectType *out) {
	YamlNode doc;
	Node *o = reinterpret_cast<Node *>(out);

	doc["Name"] = o->Name();
	doc["Children"] = YAML::Node();
	for (Node *node : o->GetChildren()) {
		doc["Children"].push_back(Serializer::get_singleton().Save(node).Yaml());
	}
	return FileBuffer(doc);
}

bool Node::LoadImpl(ObjectType *out, const FileBuffer &buf) {
	Debug::Error("Node::LoadImpl is not implemented");
	return false;
}

} // namespace Sowa
