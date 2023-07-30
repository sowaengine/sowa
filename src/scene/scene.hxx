#ifndef SW_SCENE_HXX
#define SW_SCENE_HXX
#pragma once

#include <vector>

#include "node.hxx"

class Scene {
  public:
	inline std::vector<Node *> &Nodes() { return m_nodes; }

	void BeginScene();
	void UpdateScene();
	void EndScene();

  private:
	std::vector<Node *> m_nodes;
};

#endif // SW_SCENE_HXX