#ifndef SW_TREE_NODE_HXX
#define SW_TREE_NODE_HXX
#pragma once

#include <algorithm>
#include <vector>

template <typename T, typename IDType>
class Tree;

template <typename T, typename IDType = int>
class TreeNode {
	using NodeType = TreeNode<T, IDType>;

  public:
	TreeNode() = default;
	~TreeNode() {}

	NodeType &operator=(const NodeType &o) {
		m_node = o.m_node;
		m_id = o.m_id;
		m_parentID = o.m_parentID;
		m_children = o.m_children;
		m_pTreeRef = o.m_pTreeRef;

		return *this;
	}

	inline T &Node() { return m_node; }
	inline const IDType ID() { return m_id; }

	void AddChild(IDType child) {
		auto *childNode = m_pTreeRef->FindNodeByID(child);
		if (nullptr == childNode) {
			// invalid node, throw error
			return;
		}

		if (childNode->m_id != m_sInvalidID) {
			auto *parentOfChild = m_pTreeRef->FindNodeByID(childNode->m_id);
			if (nullptr == parentOfChild) {
				// invalid parent, throw error
				return;
			}

			parentOfChild->RemoveChild(child);
		}

		m_children.push_back(child);
	}

	void RemoveChild(IDType child) {
		auto it = std::find(m_children.begin(), m_children.end(), child);
		if (it != m_children.end()) {
			m_children.erase(it);

			auto *childNode = m_pTreeRef->FindNodeByID(*it);
			if (nullptr != childNode) {
				childNode->m_parentID = m_sInvalidID;
			}
		}
	}

	NodeType *GetParent() {
		if (m_parentID == m_sInvalidID) {
			return nullptr;
		}

		auto *parent = m_pTreeRef->FindNodeByID(m_parentID);
		return parent;
	}

	std::vector<TreeNode *> GetChildren() {
		std::vector<TreeNode *> children;
		for (auto &child : m_children) {
			auto *childNode = m_pTreeRef->FindNodeByID(child);

			if (nullptr != childNode) {
				children.push_back(childNode);
			}
		}

		return children;
	}

  private:
	friend class Tree<T, IDType>;

	const IDType m_sInvalidID = {0};

	T m_node;
	IDType m_id = {0};

	IDType m_parentID = {0};
	std::vector<IDType> m_children;

	Tree<T, IDType> *m_pTreeRef = nullptr;
};

#endif // SW_TREE_NODE_HXX