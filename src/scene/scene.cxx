#include "scene.hxx"

#include "core/error/error.hxx"
#include "math/math.hxx"
#include "resource/resource_manager.hxx"
#include "scene/node_db.hxx"
#include "servers/file_server.hxx"
#include <any>
#include <iostream>

#include "yaml-cpp/yaml.h"

#include "utils/utils.hxx"

namespace YAML {
template <>
struct convert<vec2> {
	static Node encode(const vec2 &rhs) {
		Node node;
		node["x"] = (rhs.x);
		node["y"] = (rhs.y);
		return node;
	}

	static bool decode(const Node &node, vec2 &rhs) {
		if (!node.IsMap() || node.size() != 2) {
			return false;
		}

		rhs.x = node["x"].as<double>();
		rhs.y = node["y"].as<double>();
		return true;
	}
};
} // namespace YAML

namespace YAML {
template <>
struct convert<std::any> {
	static Node encode(const std::any &rhs) {
		Node node;

		if (const std::string *p = std::any_cast<std::string>(&rhs)) {
			node["str"] = *p;
		}
		if (const vec2 *p = std::any_cast<vec2>(&rhs)) {
			node["vec2"] = *p;
		}
		if (const int *p = std::any_cast<int>(&rhs)) {
			node["int"] = *p;
		}
		if (const float *p = std::any_cast<float>(&rhs)) {
			node["float"] = *p;
		}

		return node;
	}

	static bool decode(const Node &node, std::any &rhs) {
		if (!node.IsMap()) {
			return false;
		}

		std::string type = "";
		for (const auto &pair : node) {
			type = pair.first.as<std::string>();
		}

		if (type == "") {
			return false;
		}

		if (type == "str") {
			rhs = node["str"].as<std::string>("");
		} else if (type == "vec2") {
			rhs = node["vec2"].as<vec2>(vec2{0.f, 0.f});
		} else if (type == "int") {
			rhs = node["int"].as<int>(0);
		} else if (type == "float") {
			rhs = node["float"].as<float>(0.f);
		}

		return true;
	}
};
} // namespace YAML

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

Error Scene::Load(const char *path) {
	NodeDB &db = NodeDB::GetInstance();
	m_nodes.clear();

	YAML::Node node;

	std::string buffer;
	Error err = FileServer::GetInstance().ReadFileString(path, buffer);
	if (err != OK) {
		return err;
	}

	YAML::Node scene = YAML::Load(buffer);

	YAML::Node resources = scene["resources"];
	for (const auto &resource : resources) {
		std::string type = resource.second["type"].as<std::string>("");
		std::string path = resource.second["path"].as<std::string>("");
		if (type == "" || path == "") {
			continue;
		}
		RID id = resource.first.as<RID>(0);

		Resource *res = nullptr;
		if (type == "ImageTexture") {
			res = ResourceManager::GetInstance().Load(path, id, ResourceType_ImageTexture);
		}

		if (nullptr == res)
			continue;

		Utils::Info("Loaded resource ({}) id: {}", path, res->ResourceID());
	}

	YAML::Node nodes = scene["nodes"];
	std::function<void(Node *, const std::pair<YAML::Node, YAML::Node> &)> deserializeNode;

	deserializeNode = [&](Node *parent, const std::pair<YAML::Node, YAML::Node> &doc) {
		std::string name = doc.first.as<std::string>("New doc");

		YAML::Node nodeData = doc.second;
		std::string type = nodeData["type"].as<std::string>("Node");

		Node *pNode = db.Construct(db.GetNodeType(type));
		pNode->Name() = name;
		if (parent == nullptr)
			this->Nodes().push_back(pNode);
		else
			parent->AddChild(pNode);

		for (const auto &prop : nodeData["props"]) {
			std::string propName = prop.first.as<std::string>();
			std::any value = prop.second.as<std::any>();

			NodeProperty propSetter = db.GetProperty(pNode->TypeHash(), propName);
			if (propSetter.set) {
				propSetter.set(pNode, value);
			}
		}

		for (const auto &behaviour : nodeData["behaviours"]) {
			std::string name = behaviour.as<std::string>("");
			if (name == "")
				continue;

			pNode->AddBehaviour(name);
		}

		for (const auto &group : nodeData["groups"]) {
			std::string name = group.as<std::string>("");
			if (name == "")
				continue;
			pNode->get_groups().push_back(name);
		}

		for (const auto &child : nodeData["children"]) {
			deserializeNode(pNode, child);
		}
	};

	for (const auto &node : nodes) {
		deserializeNode(nullptr, node);
	}

	m_path = path;
	return OK;
}

Error Scene::Save(const char *path) {
	NodeDB &db = NodeDB::GetInstance();

	YAML::Node doc;

	YAML::Node resources;
	for (auto &[id, resource] : ResourceManager::GetInstance().Resources()) {
		YAML::Node res;

		res["type"] = "ImageTexture";
		res["path"] = resource->Filepath();
		if (resource->Type() == ResourceType_ImageTexture) {
		}

		resources[id] = res;
	}

	doc["resources"] = resources;

	YAML::Node nodes;

	std::function<void(Node *, YAML::Node &)> serializeNode;
	serializeNode = [&](Node *pNode, YAML::Node &doc) {
		YAML::Node node;
		node["type"] = db.GetNodeTypeName(pNode->TypeHash());

		YAML::Node props;
		std::vector<std::string> propNames;
		db.GetPropertyNames(pNode->TypeHash(), propNames);
		for (const auto &name : propNames) {
			props[name] = db.GetProperty(pNode->TypeHash(), name).get(pNode);
		}
		node["props"] = props;

		auto &behaviourList = pNode->GetBehaviourNames();
		if (behaviourList.size() > 0) {
			YAML::Node behaviours;

			node["behaviours"] = behaviourList;
		}

		auto groupList = pNode->get_groups();
		if (groupList.size() > 0) {
			YAML::Node groups;

			for (auto &group : groupList) {
				groups.push_back(group);
			}
			node["groups"] = groups;
		}

		doc[pNode->Name()] = node;

		for (Node *child : pNode->GetChildren()) {
			YAML::Node childrenDoc;
			serializeNode(child, childrenDoc);
			node["children"] = childrenDoc;
		}
	};

	for (Node *pNode : Nodes()) {
		serializeNode(pNode, nodes);
	}
	doc["nodes"] = nodes;

	std::string s;
	std::stringstream ss;
	ss << doc;
	s = ss.str();

	Error err = FileServer::GetInstance().WriteFileString(path, s);
	if (err != OK) {
		return err;
	}

	m_path = path;
	return OK;
}

static Node *search_node_in_group(Node *node, std::string group) {
	for (std::string &groupName : node->get_groups()) {
		if (groupName == group) {
			return node;
		}
	}

	for (Node *child : node->GetChildren()) {
		Node *find = search_node_in_group(child, group);
		if (nullptr != find)
			return find;
	}

	return nullptr;
}

Node *Scene::get_node_in_group(std::string group) {
	for (Node *node : Nodes()) {
		Node *find = search_node_in_group(node, group);
		if (nullptr != find)
			return find;
	}

	return nullptr;
}

void Scene::copy(Scene *src, Scene *dst) {
	std::function<Node *(Node *)> copyNode;

	copyNode = [&](Node *node) -> Node * {
		Node *newNode = NodeDB::GetInstance().Construct(node->TypeHash());
		std::vector<std::string> props;
		NodeDB::GetInstance().GetPropertyNames(node->TypeHash(), props);
		for (auto &propName : props) {
			NodeProperty prop = NodeDB::GetInstance().GetProperty(node->TypeHash(), propName);
			prop.set(newNode, prop.get(node));
		}

		newNode->GetBehaviourNames() = node->GetBehaviourNames();

		for (Node *child : node->GetChildren()) {
			newNode->AddChild(copyNode(child));
		}

		newNode->get_groups() = node->get_groups();

		return newNode;
	};

	dst->Nodes().clear();
	for (Node *node : src->Nodes()) {
		Node *newNode = copyNode(node);
		dst->Nodes().push_back(newNode);
	}
}