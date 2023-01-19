#ifndef _E_SCENE_HPP__
#define _E_SCENE_HPP__

#include "sowa.hpp"
#include "stlpch.hpp"

namespace Sowa {
class Node;

class Scene {
  public:
	void Start();

	Node *Create(const std::string &name);

	void UpdateLogic();
	void UpdateDraw();

	inline Node *Root() { return _Root; }

	void SetPause(bool v) { _Paused = v; }
	bool IsPaused() { return _Paused; }

  private:
	Node *_Root{nullptr};
	bool _Paused{false};
};
} // namespace Sowa

#endif // _E_SCENE_HPP__