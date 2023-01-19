#include "scene.hpp"
#include "node.hpp"

namespace Sowa {
void Scene::Start() {
}
void Scene::UpdateLogic() {
}
void Scene::UpdateDraw() {
}

Node *Scene::Create(const std::string &name) {
	Node *node = new Node(name);
	return node;
}

} // namespace Sowa
