#ifndef _E_NODE_HPP__
#define _E_NODE_HPP__

#include "sowa.hpp"
#include "stlpch.hpp"

namespace Sowa {
class Scene;

class Node {
  public:
	Node();
	Node(const std::string &name);
	virtual ~Node();

	inline std::string &Name() { return _Name; }
	inline bool IsValid() { return _pScene.lock() != nullptr; }

	// Called when entity joins / exits scene tree
	virtual void EnterScene() {}
	virtual void ExitScene() {}

	virtual void UpdateLogic() {}
	virtual void UpdateDraw() {}

  private:
	std::string _Name;

	std::weak_ptr<Scene> _pScene{};
	std::vector<Node *> _Children;
};
} // namespace Sowa

#endif // _E_NODE_HPP__