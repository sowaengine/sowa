#ifndef SW_NODE_HXX
#define SW_NODE_HXX
#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "core/behaviour/behaviour.hxx"

class Node {
  public:
	inline std::string &Name() { return m_name; }
	size_t ID() { return m_id; }

	virtual ~Node() = default;

	virtual void Start() {}
	virtual void Update() {}

	inline size_t TypeHash() { return m_typeHash; }

	void AddBehaviour(std::string name);
	void RemoveBehaviour(std::string name);
	void ReloadBehaviours();
	void StartBehaviours();
	void UpdateBehaviours();
	inline std::unordered_map<size_t, Behaviour> &GetBehaviours() { return m_behaviours; }
	inline std::vector<std::string> &GetBehaviourNames() { return m_behaviourNames; }

	//
	inline std::vector<Node *> &GetChildren() { return m_children; }
	inline Node *GetParent() { return m_parent; }

	void AddChild(Node *child);
	Node *GetChild(std::string name);
	Node *GetChildIndex(size_t index);
	void RemoveChild(std::string name);

	//
	inline std::vector<std::string> &get_groups() { return m_groups; }

  protected:
	std::string m_name = "";
	size_t m_id = 0;

  private:
	friend class NodeDB;

	void register_behaviour(const std::string &behaviour, bool callStart = false);

	size_t m_typeHash = 0;

	std::vector<std::string> m_behaviourNames;
	std::unordered_map<size_t, Behaviour> m_behaviours;

	Node *m_parent = nullptr;
	std::vector<Node *> m_children;

	std::vector<std::string> m_groups;
};

#endif // SW_NODE_HXX