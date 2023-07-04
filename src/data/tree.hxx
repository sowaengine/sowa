#ifndef SW_TREE_HXX
#define SW_TREE_HXX
#pragma once

#include "tree_node.hxx"
#include <stdint.h>
#include <unordered_map>

template <typename T, typename IDType = int>
class Tree;

template <typename T, typename IDType>
class Tree {
	using NodeType = TreeNode<T, IDType>;
	using MapType = std::unordered_map<IDType, NodeType>;

  public:
	inline MapType &Nodes() { return m_nodes; }

	template <typename... Args>
	NodeType &New(IDType id, Args... args) {
		m_nodes[id] = NodeType(args...);
		m_nodes[id].m_id = id;
		m_nodes[id].m_pTreeRef = this;

		return m_nodes[id];
	}

	void Delete(IDType id) {
		auto *node = FindNodeByID(id);
		if (nullptr != node) {
			if (nullptr != node->GetParent()) {
				node->GetParent()->RemoveChild(node);
			}
		}

		m_nodes.erase(m_nodes.find(id), m_nodes.end());
	}

	NodeType *FindNodeByID(IDType id) {
		auto it = m_nodes.find(id);
		if (it != m_nodes.end()) {
			return &(it->second);
		}

		return nullptr;
	}

  private:
	MapType m_nodes;
};

#endif // SW_TREE_HXX