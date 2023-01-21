#include "scene.hpp"
#include "debug.hpp"
#include "node.hpp"

#include "core/application.hpp"
#include "utils/random.hpp"

namespace Sowa {
Scene::~Scene() {
	Exit();
	for (Node *node : _RegisteredNodes) {
		Application::get_singleton().DestructNode(node);
	}
}

void Scene::Enter() {
	Node *root = GetRoot();

	std::function<void(Node *)> callEnter;
	callEnter = [&callEnter](Node *node) -> void {
		if (node == nullptr) {
			return;
		}

		node->EnterScene();
		for (Node *child : node->GetChildren()) {
			callEnter(child);
		}
	};
	callEnter(root);
}
void Scene::Exit() {
	Node *root = GetRoot();

	std::function<void(Node *)> callExit;
	callExit = [&callExit](Node *node) -> void {
		if (node == nullptr) {
			return;
		}

		node->ExitScene();
		for (Node *child : node->GetChildren()) {
			callExit(child);
		}
	};
	callExit(root);
}

void Scene::UpdateLogic() {
}
void Scene::UpdateDraw() {
}

void Scene::Register(Node *node) {
	_RegisteredNodes.push_back(node);
}

} // namespace Sowa
