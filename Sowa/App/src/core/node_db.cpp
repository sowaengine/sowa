#include "node_db.hpp"

namespace sowa {
NodeDB::NodeDB() {
}
NodeDB::~NodeDB() {
}

template<>
AttributeType ConvertAttributeType(int t) {
	return AttributeType::Integer;
}

template<>
AttributeType ConvertAttributeType(std::string t) {
	return AttributeType::String;
}

template<>
AttributeType ConvertAttributeType(float t) {
	return AttributeType::Float;
}

template<>
AttributeType ConvertAttributeType(bool t) {
	return AttributeType::Bool;
}

NodeDB &NodeDB::Instance() {
	static NodeDB* db = new NodeDB;
	return *db;
}

void NodeDB::RegisterNodeType(const std::string &typeName, const std::string &extendsFrom, const NodeFactory &factory) {
	auto t = NodeDB::NodeType{};
	t.typeName = typeName;
	t.extendsFrom = extendsFrom;
	t.factory = factory;

	m_types[typeName] = t;
	m_attributes[typeName].clear();
	Debug::Log("Registered type : {}", typeName);
}

Node *NodeDB::ConstructNode(const std::string &typeName) {
	auto func = m_types[typeName].factory.construct;
	if (func) {
		return func();
	}

    Debug::Error("Failed to construct node with type {}", typeName);
	return nullptr;
}
void NodeDB::DestructNode(Node *node) {
	auto func = m_types[node->GetNodeType()].factory.destruct;
	if (func) {
		func(node);
		return;
	}

	Debug::Error("Failed to destruct node with type {}", node->GetNodeType());
}

} // namespace sowa
