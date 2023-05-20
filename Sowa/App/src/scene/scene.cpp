#include "scene.hpp"
#include "debug.hpp"
#include "node.hpp"

#include "core/application.hpp"
#include "core/node_db.hpp"
#include "utils/random.hpp"
#include "serialize/serializer.hpp"
#include "serialize/document.hpp"
#include "serialize/toml_serializer.hpp"

#include <algorithm>

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

void Scene::Load(const std::string& path) {
	SerializeDocument doc = TomlSerializer::Load(path);

	uint32_t active_camera_2d = 0;
	doc.Value<uint32_t>("active_camera_2d", active_camera_2d);
	SetCurrentCamera2D(active_camera_2d);
	

	SerializeDocument nodes = doc.Table("nodes");

	
	for(const auto [key, value] : nodes) {
		if(value.type() != toml::node_type::table) {
			continue;
		}
		SerializeDocument nodeDocument = SerializeDocument(value);


		int32_t id = 0;
		auto r = std::from_chars(key.data(), key.data() + key.length(), id);
		if(r.ec == std::errc::invalid_argument) {
			id = Random::GenerateID31();
		}

		std::string name = "Node";
		std::string nodeType = "Node";

		nodeDocument.Value("name", name).Value("type", nodeType);
		Node* node = Create(nodeType, name, id);
		if(node == nullptr) {
			continue;
		}

		int32_t parentId = 0;
		nodeDocument.Value("parent", parentId);
		if(parentId == 0) {
			SetRoot(node);
		} else {
			Node* parent = GetNodeByID(parentId);
			if(parent != nullptr) {
				parent->AddChild(node);
			}
		}

		for(const auto [nodeKey, nodeValue] : nodeDocument) {
			std::string keyStr = nodeKey.data();

			if(nodeValue.is_string()) {
				NodeDB::Instance().SetAttribute(node, keyStr, nodeValue.value_or<std::string>(""));
			}
			else if(nodeValue.is_integer()) {
				NodeDB::Instance().SetAttribute(node, keyStr, nodeValue.value_or<int>(0));
			}
			else if(nodeValue.is_floating_point()) {
				NodeDB::Instance().SetAttribute(node, keyStr, nodeValue.value_or<float>(0.f));
			}
			else if(nodeValue.is_boolean()) {
				NodeDB::Instance().SetAttribute(node, keyStr, nodeValue.value_or<bool>(false));
			}
			else if(nodeValue.is_table()) {
				SerializeDocument attrTable = SerializeDocument(nodeValue);
				for(const auto [attrKey, attrValue] : attrTable) {
					std::string attrStr = attrKey.data();

					if(attrValue.is_string()) {
						NodeDB::Instance().SetAttribute(node, keyStr + "." + attrStr, attrValue.value_or<std::string>(""));
					}
					else if(attrValue.is_integer()) {
						NodeDB::Instance().SetAttribute(node, keyStr + "." + attrStr, attrValue.value_or<int>(0));
					}
					else if(attrValue.is_floating_point()) {
						NodeDB::Instance().SetAttribute(node, keyStr + "." + attrStr, attrValue.value_or<float>(0.f));
					}
					else if(attrValue.is_boolean()) {
						NodeDB::Instance().SetAttribute(node, keyStr + "." + attrStr, attrValue.value_or<bool>(false));
					}
					else {
						Debug::Error("Unknown attribute type on {}.{}", keyStr, attrStr);
					}
				}
			} else {
				Debug::Error("Unknown attribute type on {}", keyStr);
			}
		}



	}
}

Reference<Scene> Scene::New() {
	struct SceneCreator : public Scene {};

	Reference<Scene> scene = std::make_shared<SceneCreator>();
	scene->_SelfRef = scene;
	return scene;
}

Node *Scene::Create(const std::string &typeName, const std::string &name, uint32_t id) {
	Node *node = NodeDB::Instance().ConstructNode(typeName);
	if (nullptr == node) {
		return node;
	}

	node->_pScene = _SelfRef;
	node->m_id = id == 0 ? Random::GenerateID32() : id;
	node->Name() = name;
	Register(node);

	return node;
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
			Debug::Log("Delete 1 {}", node->Name());
			Application::get_singleton().DestructNode(node);
			continue;
		}

		if (!node->IsLocked() && (node->_Parent == nullptr && node != GetRoot())) {
			_RegisteredNodes.erase(_RegisteredNodes.begin() + i);
			Debug::Log("Delete 2 {}, {}", node->Name(), (void*)node->_Parent);
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
