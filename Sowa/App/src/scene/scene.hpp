#ifndef _E_SCENE_HPP__
#define _E_SCENE_HPP__

#include "sowa.hpp"
#include "stlpch.hpp"

#include "node.hpp"
#include "utils/memory.hpp"

namespace Sowa {

class Scene {
  public:
	Scene() = default;
	~Scene();

	void Enter();
	void Exit();

	void UpdateLogic();
	void UpdateDraw();

	template <typename T>
	T *Create(const std::string &name) {
		std::allocator<T> allocator = Allocator<T>::Get();
		T *node = allocator.allocate(1);
		node->Name() = name;
		Register(node);

		return node;
	}

	void Register(Node *node);

	void SetPause(bool v) { _Paused = v; }
	bool IsPaused() { return _Paused; }

	inline void SetRoot(Node *node) { _Root = node; }
	inline Node *GetRoot() { return _Root; }

  private:
	bool _Paused{false};

	std::vector<Node *> _RegisteredNodes{};

	Node *_Root{nullptr};
};
} // namespace Sowa

#endif // _E_SCENE_HPP__