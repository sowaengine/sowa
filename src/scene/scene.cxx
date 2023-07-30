#include "scene.hxx"

void Scene::BeginScene() {
	for (Node *node : m_nodes) {
		node->Start();
	}
}

void Scene::UpdateScene() {
	for (Node *node : m_nodes) {
		node->Update();
	}
}

void Scene::EndScene() {
	// todo
}