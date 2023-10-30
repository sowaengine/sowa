#ifndef SW_NODE_DB_HXX
#define SW_NODE_DB_HXX
#pragma once

#include <any>
#include <functional>
#include <iostream>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include "nodes/node.hxx"
#include "utils/utils.hxx"

// using Property = std::variant<std::string>;
using Property = std::any;
typedef size_t NodeType;

struct NodeFactory {
	std::function<Node *()> constructor;
	std::function<void(Node *)> destructor;
};

struct NodeProperty {
	std::function<Property(Node *)> get;
	std::function<Property(Node *)> get_ref;
	std::function<void(Node *, Property)> set;
	std::string typeName = "";
};

struct NodeData {
	NodeFactory factory;
	NodeType extends;

	std::unordered_map<std::string, NodeProperty> properties;
};

class NodeDB {
  public:
	static NodeDB &get();

	template <typename T>
	NodeType bind_node_type(std::string nodeType, NodeType extends, NodeFactory factory) {
		NodeType hash = typeid(T).hash_code();

		NodeData data;
		data.factory = factory;
		data.extends = extends;
		m_db[hash] = data;

		m_node_type_hashes[nodeType] = hash;
		m_node_typenames[hash] = nodeType;

		return hash;
	}

	void bind_property(NodeType type, std::string propertyName, NodeProperty property) {
		m_db[type].properties[propertyName] = property;
	}

	NodeProperty get_property(NodeType type, std::string propertyName) {
		if (m_db.find(type) == m_db.end())
			return NodeProperty();

		NodeData &data = m_db[type];
		if (data.properties.find(propertyName) == data.properties.end())
			return get_property(data.extends, propertyName);

		NodeProperty &prop = data.properties[propertyName];
		if (!prop.get && !prop.set) {
			if (data.extends != 0) {
				return get_property(data.extends, propertyName);
			}
		}

		return prop;
	}

	NodeType get_node_type(std::string nodeType) {
		if (nodeType == "") {
			return 0;
		}
		return m_node_type_hashes[nodeType];
	}

	std::string get_node_typename(NodeType type) {
		return m_node_typenames[type];
	}

	void get_property_names(NodeType type, std::vector<std::string> &names, bool clear = true) {
		if (clear) {
			names.clear();
		}

		for (const auto &[name, prop] : m_db[type].properties) {
			names.push_back(name);
		}

		NodeType extends = m_db[type].extends;
		if (extends != 0) {
			get_property_names(extends, names, false);
		}
	}

	Node *construct(NodeType type, const std::string &name = "", size_t id = 0) {
		std::function<Node *()> constructor = m_db[type].factory.constructor;

		if (constructor) {
			Node *node = constructor();
			node->m_type_hash = type;
			node->name() = name == "" ? "New Node" : name;
			if (id == 0) {
				id = Utils::RandRange(1, 1'000'000);
			}
			node->m_id = id;

			return node;
		}

		Utils::Error("Invalid node type: ", (int)type);
		return nullptr;
	}

	void destruct(Node *node) {
		std::function<void(Node *)> destructor = m_db[node->m_type_hash].factory.destructor;
		if (destructor) {
			destructor(node);
		}
	}

  private:
	friend class ScriptServer;
	NodeDB();
	~NodeDB();

	std::unordered_map<NodeType, NodeData> m_db;
	std::unordered_map<std::string, NodeType> m_node_type_hashes;
	std::unordered_map<NodeType, std::string> m_node_typenames;
};

#endif // SW_NODE_DB_HXX