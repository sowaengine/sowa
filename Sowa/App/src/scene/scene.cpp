#include "scene.hpp"
#include "debug.hpp"
#include "node.hpp"

#include "core/application.hpp"
#include "utils/random.hpp"

namespace sowa {
Scene::Scene() {
	m_type = Typename();
}
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
	for (Node *node : _RegisteredNodes) {
		node->UpdateLogic();
	}
}
void Scene::UpdateDraw() {
	for (Node *node : _RegisteredNodes) {
		node->UpdateDraw();
	}
}

Reference<Scene> Scene::New() {
	struct SceneCreator : public Scene {};

	Reference<Scene> scene = std::make_shared<SceneCreator>();
	scene->_SelfRef = scene;
	return scene;
}

Node *Scene::GetNodeByID(uint32_t id) {
	for (Node *node : _RegisteredNodes) {
		if (node->ID() == id) {
			return node;
		}
	}
	return nullptr;
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

FileBuffer Scene::SaveImpl(object_type *out) {
	YamlNode doc;
	Scene *o = reinterpret_cast<Scene *>(out);

	YAML::Node nodes;
	if (o->GetRoot() != nullptr) {
		nodes = Serializer::get_singleton().Save(o->GetRoot()).Yaml();
	}
	if (!nodes) {
		nodes = "";
	}

	doc["Name"] = "mainscene";
	doc["Nodes"] = nodes;
	return FileBuffer(doc);
}

bool Scene::LoadImpl(object_type *out, const FileBuffer &buf) {
	Debug::Error("Scene::LoadImpl is not implemented");
	return false;
}

} // namespace sowa
