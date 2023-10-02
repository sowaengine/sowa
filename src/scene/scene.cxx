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
		} else if (const vec2 *p = std::any_cast<vec2>(&rhs)) {
			node["vec2"] = *p;
		} else if (const int *p = std::any_cast<int>(&rhs)) {
			node["int"] = *p;
		} else if (const float *p = std::any_cast<float>(&rhs)) {
			node["float"] = *p;
		} else if (const bool *p = std::any_cast<bool>(&rhs)) {
			node["bool"] = *p;
		} else {
			assert(false && "unknown type on std::any encode");
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
		} else if (type == "bool") {
			rhs = node["bool"].as<bool>(false);
		} else {
			assert(false && "unknown type on std::any encode");
		}

		return true;
	}
};
} // namespace YAML

void Scene::_begin_scene() {
	for (Node *node : m_nodes) {
		node->_start();
	}
}

void Scene::_update_scene() {
	for (Node *node : m_nodes) {
		node->_update();
	}
}

void Scene::_end_scene() {
	for (Node *node : m_nodes) {
		node->_exit();
	}
}

ErrorCode Scene::load(const char *path) {
	NodeDB &db = NodeDB::get();
	m_nodes.clear();

	YAML::Node node;

	std::string buffer;
	ErrorCode err = FileServer::get().ReadFileString(path, buffer);
	if (err != OK) {
		return err;
	}

	YAML::Node scene = YAML::Load(buffer);

	m_active_camera_2d = scene["active_camera_2d"].as<size_t>(0);

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
			res = load_resource(path, id, ResourceType_ImageTexture);
		} else if (type == "AudioStream") {
			res = load_resource(path, id, ResourceType_AudioStream);
		} else if (type == "SpriteSheetAnimation") {
			res = load_resource(path, id, ResourceType_SpriteSheetAnimation);
		} else {
			res = load_resource(path, id, ResourceType_None);
			assert(nullptr != res && "unknown resource type");
		}

		if (nullptr == res)
			continue;

		Utils::Info("Loaded resource ({}) id: {}", path, res->ResourceID());
	}

	YAML::Node nodes = scene["nodes"];
	std::function<void(Node *, const std::pair<YAML::Node, YAML::Node> &)> deserializeNode;

	deserializeNode = [&](Node *parent, const std::pair<YAML::Node, YAML::Node> &doc) {
		size_t id = doc.first.as<size_t>(0);
		YAML::Node nodeData = doc.second;
		std::string type = nodeData["type"].as<std::string>("Node");
		std::string name = nodeData["name"].as<std::string>("New Node");

		Node *pNode = create(db.get_node_type(type), name, id);
		if (parent == nullptr)
			this->Nodes().push_back(pNode);
		else
			parent->add_child(pNode);

		for (const auto &prop : nodeData["props"]) {
			std::string propName = prop.first.as<std::string>();
			if (propName == "name")
				continue;

			std::any value = prop.second.as<std::any>();

			NodeProperty propSetter = db.get_property(pNode->type_hash(), propName);
			if (propSetter.set) {
				propSetter.set(pNode, value);
			}
		}

		for (const auto &behaviour : nodeData["behaviours"]) {
			std::string name = behaviour.as<std::string>("");
			if (name == "")
				continue;

			pNode->add_behaviour(name);
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

ErrorCode Scene::save(const char *path) {
	NodeDB &db = NodeDB::get();

	YAML::Node doc;

	doc["active_camera_2d"] = m_active_camera_2d;

	YAML::Node resources;
	for (const RID &id : scene_resources()) {
		Resource *resource = ResourceManager::get().Get(id);
		YAML::Node res;

		if (resource->Type() == ResourceType_ImageTexture) {
			res["type"] = "ImageTexture";
		} else if (resource->Type() == ResourceType_AudioStream) {
			res["type"] = "AudioStream";
		} else if (resource->Type() == ResourceType_SpriteSheetAnimation) {
			res["type"] = "SpriteSheetAnimation";
		} else {
			assert(false && "unknown resource type");
		}
		res["path"] = resource->Filepath();

		resources[id] = res;
	}

	doc["resources"] = resources;

	YAML::Node nodes;

	std::function<void(Node *, YAML::Node &)> serializeNode;
	serializeNode = [&](Node *pNode, YAML::Node &doc) {
		YAML::Node node;
		node["type"] = db.get_node_typename(pNode->type_hash());
		node["name"] = pNode->name();

		YAML::Node props;
		std::vector<std::string> propNames;
		db.get_property_names(pNode->type_hash(), propNames);
		for (const auto &name : propNames) {
			if (name == "name")
				continue;

			props[name] = db.get_property(pNode->type_hash(), name).get(pNode);
		}
		node["props"] = props;

		auto &behaviourList = pNode->get_behaviour_names();
		if (behaviourList.size() > 0) {
			YAML::Node behaviours;

			node["behaviours"] = behaviourList;
		}

		auto groupList = pNode->get_groups();
		if (groupList.size() > 0) {
			YAML::Node groups;
			groups = groupList;

			node["groups"] = groups;
		}

		doc[pNode->id()] = (node);

		YAML::Node childrenDoc;
		for (Node *child : pNode->get_children()) {
			serializeNode(child, childrenDoc);
		}
		if (childrenDoc.size() > 0)
			node["children"] = childrenDoc;
	};

	for (Node *pNode : Nodes()) {
		serializeNode(pNode, nodes);
	}
	doc["nodes"] = nodes;

	std::string s;
	std::stringstream ss;
	ss << doc;
	s = ss.str();

	ErrorCode err = FileServer::get().WriteFileString(path, s);
	if (err != OK) {
		return err;
	}

	m_path = path;
	return OK;
}

Node *Scene::create(NodeType type, const std::string &name, size_t id) {
	Node *node = NodeDB::get().construct(type, name, id);
	m_allocated_nodes[node->id()] = node;

	return node;
}

Resource *Scene::load_resource(const std::string &path, RID id, ResourceType type) {
	Resource *res = ResourceManager::get().Load(path, id, type);
	m_resources.push_back(res->ResourceID());

	return res;
}

Camera2D *Scene::get_active_camera2d() {
	return dynamic_cast<Camera2D *>(get_node_by_id(m_active_camera_2d));
}

static Node *search_node_in_group(Node *node, std::string group) {
	for (std::string &groupName : node->get_groups()) {
		if (groupName == group) {
			return node;
		}
	}

	for (Node *child : node->get_children()) {
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

Node *Scene::get_node_by_id(size_t id) {
	if (m_allocated_nodes.find(id) == m_allocated_nodes.end())
		return nullptr;

	return m_allocated_nodes[id];
}

void Scene::copy(Scene *src, Scene *dst) {
	std::function<Node *(Node *)> copyNode;

	copyNode = [&](Node *node) -> Node * {
		Node *newNode = dst->create(node->type_hash(), node->name(), node->id());
		std::vector<std::string> props;
		NodeDB::get().get_property_names(node->type_hash(), props);
		for (auto &propName : props) {
			NodeProperty prop = NodeDB::get().get_property(node->type_hash(), propName);
			prop.set(newNode, prop.get(node));
		}

		newNode->get_behaviour_names() = node->get_behaviour_names();

		for (Node *child : node->get_children()) {
			newNode->add_child(copyNode(child));
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