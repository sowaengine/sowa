#include "scene_tree.hxx"

#include "core/app.hxx"

SceneTree &SceneTree::get() {
	static SceneTree *tree = new SceneTree;
	return *tree;
}

SceneTree::SceneTree() {
	m_scene = new Scene;
}

SceneTree::~SceneTree() {
	delete m_scene;
}

void SceneTree::_begin_globals() {
	for (Node *node : m_global_nodes) {
		node->_start();
		node->start_behaviours();
	}
}

void SceneTree::_update_globals() {
	for (Node *node : m_global_nodes) {
		node->_update();
		if (App::get().IsRunning())
			node->update_behaviours();
	}
}

void SceneTree::_end_globals() {
	for (Node *node : m_global_nodes) {
		node->_exit();
	}
}