#ifndef SW_NODE_DB_HPP_
#define SW_NODE_DB_HPP_

#include <functional>
#include <string>
#include <unordered_map>

#include "scene/scene.hpp"

namespace sowa {

struct NodeFactory {
	std::function<Node *()> construct;
	std::function<void(Node *)> destruct;
};

/*
	Holds node types
	Can be used for template nodes
*/
class NodeDB {
  public:
	static NodeDB &Instance();

	void RegisterNodeType(const std::string &typeName, const std::string &extendsFrom, const NodeFactory &factory);

	Node* ConstructNode(const std::string& typeName);
	void DestructNode(Node* node);

  private:
	NodeDB();
	~NodeDB();

	struct NodeType {
		std::string typeName;
		std::string extendsFrom;
		NodeFactory factory;
	};

	std::unordered_map<std::string, NodeType> m_types;
};
} // namespace sowa

#endif