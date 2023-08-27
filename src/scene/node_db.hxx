#ifndef SW_NODE_DB_HXX
#define SW_NODE_DB_HXX
#pragma once

#include <any>
#include <functional>
#include <iostream>
#include <unordered_map>
#include <variant>
#include <vector>

#include "node.hxx"

// using Property = std::variant<std::string>;
using Property = std::any;
typedef size_t NodeType;

struct NodeFactory {
	std::function<Node *()> constructor;
	std::function<void(Node *)> destructor;
};

struct NodeProperty {
	std::function<Property(Node *)> get;
	std::function<void(Node *, Property)> set;
};

struct NodeData {
	NodeFactory factory;
	NodeType extends;

	std::unordered_map<std::string, NodeProperty> properties;
};

class NodeDB {
  public:
	static NodeDB &GetInstance();

	template <typename T>
	NodeType BindNodeType(std::string nodeType, NodeType extends, NodeFactory factory) {
		NodeType hash = typeid(T).hash_code();

		NodeData data;
		data.factory = factory;
		data.extends = extends;
		m_db[hash] = data;

		m_nodeTypeHashes[nodeType] = hash;
		m_nodeTypeNames[hash] = nodeType;

		return hash;
	}

	void BindProperty(NodeType type, std::string propertyName, NodeProperty property) {
		m_db[type].properties[propertyName] = property;
	}

	NodeProperty GetProperty(NodeType type, std::string propertyName) {
		NodeProperty prop = m_db[type].properties[propertyName];
		if (!prop.get && !prop.set) {
			NodeType extends = m_db[type].extends;
			if (extends != 0) {
				return GetProperty(extends, propertyName);
			}
		}

		return prop;
	}

	NodeType GetNodeType(std::string nodeType) {
		return m_nodeTypeHashes[nodeType];
	}

	std::string GetNodeTypeName(NodeType type) {
		return m_nodeTypeNames[type];
	}

	void GetPropertyNames(NodeType type, std::vector<std::string> &names, bool clear = true) {
		if (clear) {
			names.clear();
		}

		for (const auto &[name, prop] : m_db[type].properties) {
			names.push_back(name);
		}

		NodeType extends = m_db[type].extends;
		if (extends != 0) {
			GetPropertyNames(extends, names, false);
		}
	}

	Node *Construct(NodeType type) {
		std::function<Node *()> constructor = m_db[type].factory.constructor;

		if (constructor) {
			Node *node = constructor();
			node->m_typeHash = type;
			node->Name() = "New Node";

			return node;
		}

		return nullptr;
	}

	void Destruct(Node *node) {
		std::function<void(Node *)> destructor = m_db[node->m_typeHash].factory.destructor;
		if (destructor) {
			destructor(node);
		}
	}

  private:
	NodeDB();
	~NodeDB();

	std::unordered_map<NodeType, NodeData> m_db;
	std::unordered_map<std::string, NodeType> m_nodeTypeHashes;
	std::unordered_map<NodeType, std::string> m_nodeTypeNames;
};

#endif // SW_NODE_DB_HXX