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

  protected:
	std::string m_name = "";
};

#endif // SW_NODE_HXX