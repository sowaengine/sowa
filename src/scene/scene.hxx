#ifndef SW_SCENE_HXX
#define SW_SCENE_HXX
#pragma once

#include <vector>

#include "core/error/error.hxx"
#include "node.hxx"

class Scene {
  public:
	inline std::vector<Node *> &Nodes() { return m_nodes; }

	virtual void BeginScene();
	virtual void UpdateScene();
	virtual void EndScene();

  private:
	std::vector<Node *> m_nodes;
};

#endif // SW_SCENE_HXX