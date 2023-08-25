#ifndef SW_NODE_HXX
#define SW_NODE_HXX
#pragma once

#include <string>

class Node {
  public:
	inline std::string &Name() { return m_name; }
	virtual ~Node() = default;

	virtual void Start() {}
	virtual void Update() {}

	inline size_t TypeHash() { return m_typeHash; }

  protected:
	std::string m_name = "";

  private:
	friend class NodeDB;
	size_t m_typeHash = 0;
};

#endif // SW_NODE_HXX