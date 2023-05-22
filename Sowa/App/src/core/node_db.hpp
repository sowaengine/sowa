#ifndef SW_NODE_DB_HPP_
#define SW_NODE_DB_HPP_

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

#include "scene/scene.hpp"

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
	AttributeType attrType;
};

template <typename T>
using GetterFunc = T (*)(Node *);

template <typename T>
using SetterFunc = void (*)(Node *, T);

/*
	Holds node types
	Can be used for template nodes
*/
class NodeDB {
  public:
	static NodeDB &Instance();

	void RegisterNodeType(const std::string &typeName, const std::string &extendsFrom, const NodeFactory &factory);

	template <typename T>
	void RegisterAttribute(const std::string &typeName, const std::string &attr, GetterFunc<T> getter, SetterFunc<T> setter) {
		AttributeModifier m;
		m.getterFunc = (void *)getter;
		m.setterFunc = (void *)setter;
		m.attrType = ConvertAttributeType<T>();

		m_attributes[typeName][attr] = m;
	}

	template <typename T>
	void SetAttribute(Node *node, std::string attr, T value, std::string nodeType = "") {
		if(nodeType == "") {
			nodeType = node->GetNodeType();
		}

		SetterFunc<T> f = reinterpret_cast<SetterFunc<T>>(m_attributes[nodeType][attr].setterFunc);
		if (f) {
			f(node, value);
			Debug::Log("{}.{} = {}", node->Name(), attr, value);
		} else {
			std::string extendsFrom = m_types[nodeType].extendsFrom;
			if(extendsFrom != "") {
				SetAttribute<T>(node, attr, value, extendsFrom);
			}
		}
	}

	bool IsInstanceOf(std::string nodeType, std::string instanceOf) {
		if(nodeType == instanceOf) {
			return true;
		}
		std::string extendsFrom = m_types[nodeType].extendsFrom;
		if(extendsFrom == "") {
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