#include "scene.hpp"
#include "debug.hpp"
#include "node.hpp"

#include "core/application.hpp"
#include "utils/random.hpp"

namespace Sowa {
Scene::Scene() {}
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

Reference<Scene> Scene::New() {
	struct SceneCreator : public Scene {};

	Reference<Scene> scene = std::make_shared<SceneCreator>();
	scene->_SelfRef = scene;
	return scene;
}

void Scene::Register(Node *node) {
	_RegisteredNodes.push_back(node);
}

void Scene::CollectNodes() {
	for (size_t i = 0; i < _RegisteredNodes.size();) {
		Node *node = _RegisteredNodes[i];
		if (node == nullptr) {
			_RegisteredNodes.erase(_RegisteredNodes.begin() + i);
			continue;
		}
		if (!node->IsLocked() && !node->IsValid()) {
			_RegisteredNodes.erase(_RegisteredNodes.begin() + i);
			Application::get_singleton().DestructNode(node);
			continue;
		}

		if (!node->IsLocked() && (node->_Parent == nullptr && node != GetRoot())) {
			_RegisteredNodes.erase(_RegisteredNodes.begin() + i);
			Application::get_singleton().DestructNode(node);
			continue;
		}

		i++;
	}
}

} // namespace Sowa
