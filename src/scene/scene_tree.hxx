#ifndef SW_SCENE_TREE_HXX
#define SW_SCENE_TREE_HXX

#include "scene.hxx"

class SceneTree {
  public:
	static SceneTree &get();

	inline Node *root() { return m_scene->Root(); }
	inline std::vector<Node *> &global_nodes() { return m_global_nodes; }

	void _begin_globals();
	void _update_globals();
	void _end_globals();

	inline Scene *get_scene() { return m_scene; }

	SceneTree();
	~SceneTree();

  private:
	friend class App;
	std::vector<Node *> m_global_nodes;

	Scene *m_scene = nullptr;
};

#endif // SW_SCENE_TREE_HXX