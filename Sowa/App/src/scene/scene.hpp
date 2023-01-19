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

  private:
	Node *_Root;
};
} // namespace Sowa

#endif // _E_SCENE_HPP__