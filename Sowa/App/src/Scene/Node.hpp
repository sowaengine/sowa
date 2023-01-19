#ifndef _E_NODE_HPP__
#define _E_NODE_HPP__

#include "Sowa.hpp"
#include "stlpch.hpp"

namespace Sowa {
class Scene;

class Node {
  public:
	inline std::string &Name() { return _Name; }
	inline bool IsValid() { return _pScene.lock() != nullptr; }

  private:
	std::string _Name;

	std::weak_ptr<Scene> _pScene{};
	std::vector<Node *> _Children;
};
} // namespace Sowa

#endif // _E_NODE_HPP__