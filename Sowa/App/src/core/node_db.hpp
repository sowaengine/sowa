#ifndef SW_NODE_DB_HPP_
#define SW_NODE_DB_HPP_

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

#include "scene/scene.hpp"
#include "utils/light_variant.hpp"

namespace sowa {

struct NodeFactory {
	std::function<Node *()> construct;
	std::function<void(Node *)> destruct;
};

enum class AttributeType {
	Integer = 0,
	String,
	Float,
	Bool
};
template <typename T>
AttributeType ConvertAttributeType(T t = T());

struct AttributeModifier {
	void *setterFunc;
	void *getterFunc;
};

using GetterFunc = light_variant (*)(Node *);

using SetterFunc = void (*)(Node *, light_variant);

/*
	Holds node types
	Can be used for template nodes
*/
class NodeDB {
  public:
	static NodeDB &Instance();

	void RegisterNodeType(const std::string &typeName, const std::string &extendsFrom, const NodeFactory &factory);

	void RegisterAttribute(const std::string &typeName, const std::string &attr, GetterFunc getter, SetterFunc setter) {
		AttributeModifier m;
		m.getterFunc = (void *)getter;
		m.setterFunc = (void *)setter;

		m_attributes[typeName][attr] = m;
	}

	void SetAttribute(Node *node, std::string attr, light_variant value, std::string nodeType = "") {
		if (nodeType == "") {
			nodeType = node->GetNodeType();
		}

		bool lookup = true;
		if (m_attributes.find(nodeType) == m_attributes.end()) {
			return;
		}
		if (m_attributes[nodeType].find(attr) == m_attributes[nodeType].end()) {
			lookup = false;
		}

		SetterFunc f = nullptr;
		if (lookup)
			f = reinterpret_cast<SetterFunc>(m_attributes[nodeType][attr].setterFunc);
		if (lookup && f) {
			f(node, value);
		} else {
			std::string extendsFrom = m_types[nodeType].extendsFrom;
			if (extendsFrom != "") {
				SetAttribute(node, attr, value, extendsFrom);
			}
		}
	}

	light_variant GetAttribute(Node *node, std::string attr, std::string nodeType = "") {
		if (nodeType == "") {
			nodeType = node->GetNodeType();
		}

		bool lookup = true;
		if (m_attributes.find(nodeType) == m_attributes.end()) {
			return light_variant(false);
		}
		if (m_attributes[nodeType].find(attr) == m_attributes[nodeType].end()) {
			lookup = false;
		}

		GetterFunc f = nullptr;
		if (lookup)
			f = reinterpret_cast<GetterFunc>(m_attributes[nodeType][attr].getterFunc);
		if (lookup && f) {
			return f(node);
		} else {
			std::string extendsFrom = m_types[nodeType].extendsFrom;
			if (extendsFrom != "") {
				return GetAttribute(node, attr, extendsFrom);
			}
		}
		return light_variant(false);
	}

	void GetAttributeList(Node *node, std::vector<std::string> &list, std::string nodeType = "") {
		if (nodeType == "") {
			nodeType = node->GetNodeType();
		}

		for (auto &[attrName, attrModifier] : m_attributes[nodeType]) {
			list.push_back(attrName);
		}
		std::string extendsFrom = m_types[nodeType].extendsFrom;
		if (extendsFrom != "") {
			GetAttributeList(node, list, extendsFrom);
		}
	}

	bool IsInstanceOf(std::string nodeType, std::string instanceOf) {
		if (nodeType == instanceOf) {
			return true;
		}
		std::string extendsFrom = m_types[nodeType].extendsFrom;
		if (extendsFrom == "") {
			return false;
		}

		return IsInstanceOf(extendsFrom, instanceOf);
	}

	Node *ConstructNode(const std::string &typeName);
	void DestructNode(Node *node);

  private:
	NodeDB();
	~NodeDB();
	NodeDB(const NodeDB &) = delete;
	NodeDB(const NodeDB &&) = delete;
	NodeDB &operator=(const NodeDB &) = delete;
	NodeDB &operator=(const NodeDB &&) = delete;

	struct NodeType {
		std::string typeName;
		std::string extendsFrom;
		NodeFactory factory;
	};

	std::unordered_map<std::string, NodeType> m_types;
	std::unordered_map<std::string, std::unordered_map<std::string, AttributeModifier>> m_attributes;
};
} // namespace sowa

#endif