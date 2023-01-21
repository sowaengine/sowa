#ifndef _E_NODE_HPP__
#define _E_NODE_HPP__

#include "sowa.hpp"
#include "stlpch.hpp"

namespace Sowa {
class Scene;
class Application;

enum class PauseMode {
	Inherit = 0,
	Ignore,
	Stop,
};

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

	inline PauseMode GetPauseMode() { return _PauseMode; }
	void SetPauseMode(PauseMode mode) { _PauseMode = mode; }

	inline std::vector<Node *> GetChildren() { return _Children; }

  private:
	friend class Scene;
	friend class Application;
	std::string _Name;

	std::weak_ptr<Scene> _pScene{};
	std::vector<Node *> _Children;
	Node *_Parent;

	PauseMode _PauseMode{PauseMode::Inherit};

  protected:
	std::string _NodeType{"Node"};
};
} // namespace Sowa

#endif // _E_NODE_HPP__