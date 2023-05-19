#include "node.hpp"
#include "scene.hpp"
#include "serialize/serializer.hpp"

#include "core/node_db.hpp"

namespace sowa {
Node::Node() : _Name("Node") {
	m_type = Typename();
}
Node::Node(const std::string &name) : _Name(name) {}
Node::~Node() {
	if (IsValid()) {
	}
}

void Node::Bind() {
	NodeFactory factory;
	factory.construct = []() -> Node* {
		Node* node = Allocator<Node>::Get().allocate(1);
		new (node) Node;

		return node;
	};

	factory.destruct = [](Node* node) {
		Allocator<Node>::Get().deallocate(reinterpret_cast<Node *>(node), 1);
	};

	NodeDB::Instance().RegisterNodeType("Node", "", factory);


	Serializer::get_singleton().RegisterSerializer(Node::Typename(), SerializeImpl(Node::SaveImpl, Node::LoadImpl));
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
			if (node->GetParent() == this) {
				_Children.erase(_Children.begin() + i);
				node->SetParent(nullptr);
			}
			return;
		}
	}
}

void Node::SetParent(Node *parent) {
	if (_Parent != nullptr && _Parent != parent)
		_Parent->RemoveNode(this);
	
	_Parent = parent;
}

FileBuffer Node::SaveImpl(object_type *out) {
	YamlNode doc;
	Node *o = reinterpret_cast<Node *>(out);

	doc["Name"] = o->Name();
	doc["Type"] = o->GetNodeType();
	doc["Children"] = YAML::Node();
	for (Node *node : o->GetChildren()) {
		doc["Children"].push_back(Serializer::get_singleton().Save(node).Yaml());
	}
	return FileBuffer(doc);
}

bool Node::LoadImpl(object_type *out, const FileBuffer &buf) {
	Debug::Error("Node::LoadImpl is not implemented");

	Node *o = reinterpret_cast<Node *>(out);
	YamlNode doc = buf.Yaml();
	o->Name() = doc["Name"].as<std::string>("Node");

	YamlNode children = doc["Children"];
	for(YamlNode::const_iterator it = children.begin(); it != children.end(); ++it) {
		// Node* node = Scene::Create("Type", "Name");
		// if(!Serializer::Load(node, *it)) {
		// 		return false;
		// }
		// AddChild(node);
	}
	/*
		Application::RegisterNodeType(nodeType, constructFunc, destructFunc);
	*/

	return true;
}

} // namespace sowa
