#ifndef SW_NODE_HXX
#define SW_NODE_HXX
#pragma once

#include <string>
#include <unordered_map>

#include "core/behaviour/behaviour.hxx"

class Node {
  public:
	inline std::string &Name() { return m_name; }
	virtual ~Node() = default;

	virtual void Start() {}
	virtual void Update() {}

	inline size_t TypeHash() { return m_typeHash; }

	void AddBehaviour(std::string name);
	void RemoveBehaviour(std::string name);
	void UpdateBehaviours();

  protected:
	std::string m_name = "";

  private:
	friend class NodeDB;
	size_t m_typeHash = 0;

	std::unordered_map<size_t, Behaviour> m_behaviours;
};

#endif // SW_NODE_HXX