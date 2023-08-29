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

	Error Load(const char *path);
	Error Save(const char *path);

	Node *get_node_in_group(std::string group);
	inline const std::string &Path() { return m_path; }

	static void copy(Scene *src, Scene *dst);

  private:
	std::vector<Node *> m_nodes;

	std::string m_path = "";
};

#endif // SW_SCENE_HXX