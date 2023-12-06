#include "scene.hxx"

#include "core/app.hxx"
#include "core/error/error.hxx"
#include "core/tweens.hxx"
#include "math/math.hxx"
#include "resource/resource_manager.hxx"
#include "scene/node_db.hxx"
#include "scene/nodes/2d/physics/physics_body_2d.hxx"
#include "servers/file_server.hxx"
#include "utils/utils.hxx"

#include <any>
#include <iostream>

#include "google/protobuf/text_format.h"
#include "proto/scene.pb.h"

void Scene::_begin_scene() {
	// _awake, _start
	// separate _start and start_behaviours
	for (auto &[id, node] : m_allocated_nodes) {
		node->_start();
		node->start_behaviours();
	}
}

void Scene::_update_scene() {
	for (auto id : m_nodes_to_free) {
		this->free(id);
	}
	m_nodes_to_free.clear();

	for (auto &[id, node] : m_allocated_nodes) {
		node->_update();
		if (App::get().IsRunning())
			node->update_behaviours();
	}
}

void Scene::_end_scene() {
	Tweens::get().clear();

	for (auto &[id, node] : m_allocated_nodes) {
		node->_exit();
	}
}

ErrorCode Scene::load(const char *path) {
	NodeDB &db = NodeDB::get();

	if (nullptr != Root()) {
		this->free(Root()->id());
		Root() = nullptr;
	}
	m_allocated_nodes.clear();

	// for (int id : m_resources) {
	// 	ResourceManager::get().Unload(id);
	// }
	// m_resources.clear();

	pb::Scene scn;
	Result<file_buffer *> buf = FileServer::get().load_file(path, ReadWriteFlags_AsText);
	if (!buf.ok()) {
		return buf.error();
	}

	std::string v((char *)buf.get()->data(), buf.get()->size());
	google::protobuf::TextFormat::ParseFromString(v, &scn);

	m_active_camera_2d = scn.active_camera_2d();
	for (int i = 0; i < scn.resource_size(); i++) {
		pb::Resource *r = scn.mutable_resource(i);

		uint64_t type = r->type();
		std::string path = r->path();
		if (path == "")
			continue;

		RID id = r->id();

		Resource *res = nullptr;
		res = load_resource(path, id, type);

		if (nullptr == res) {
			Utils::Error("Failed to load resource {}", path);
			continue;
		}
	}

	// parentIds[nodeId] = node.parentID
	std::unordered_map<uint64_t, uint64_t> parentIds;
	for (int i = 0; i < scn.node_size(); i++) {
		pb::Node *n = scn.mutable_node(i);

		uint64_t id = n->id();
		uint64_t parentId = n->parent_id();
		std::string type = n->type();
		std::string name = n->name();

		Node *node = create(db.get_node_type(type), name, id);
		parentIds[id] = parentId;

		for (int j = 0; j < n->behaviour_size(); j++) {
			node->add_behaviour(n->behaviour(j));
		}
		for (int j = 0; j < n->group_size(); j++) {
			node->add_group(n->group(j));
		}
		std::vector<std::string> propNames;
		db.get_property_names(node->type_hash(), propNames);
		for (std::string &propName : propNames) {
			const google::protobuf::FieldDescriptor *field = n->GetDescriptor()->FindFieldByName(propName);

			if (nullptr == field)
				continue;

			NodeProperty prop = db.get_property(node->type_hash(), propName);

			switch (field->type()) {
			case google::protobuf::FieldDescriptor::TYPE_BOOL:
				prop.set(node, n->GetReflection()->GetBool(*n, field));
				break;
			case google::protobuf::FieldDescriptor::TYPE_INT32:
				prop.set(node, n->GetReflection()->GetInt32(*n, field));
				break;
			case google::protobuf::FieldDescriptor::TYPE_FLOAT:
				prop.set(node, n->GetReflection()->GetFloat(*n, field));
				break;
			case google::protobuf::FieldDescriptor::TYPE_STRING: {

				std::string v = n->GetReflection()->GetString(*n, field);
				if (field->name() == "body_type")
					prop.set(node, StringToPhysicsBodyType(v));
				else
					prop.set(node, v);

			} break;
			case google::protobuf::FieldDescriptor::TYPE_MESSAGE: {

				std::string msgName = field->message_type()->name();
				if (msgName == "Vec2") {
					const google::protobuf::Message &v = n->GetReflection()->GetMessage(*n, field);

					auto des = v.GetDescriptor();
					auto refl = v.GetReflection();

					auto fieldX = des->FindFieldByName("x");
					auto fieldY = des->FindFieldByName("y");
					float x = refl->GetFloat(v, fieldX);
					float y = refl->GetFloat(v, fieldY);

					prop.set(node, vec2(x, y));
				} else {
					Utils::Error("Unknown message type: {}", field->message_type()->name());
				}

			} break;

			default:
				Utils::Error("Unknown field type name: {}", field->type_name());
				break;
			}
		}
	}

	Root() = get_node_by_id(scn.root_id());
	for (auto &[id, parentID] : parentIds) {
		Node *parent = get_node_by_id(parentID);
		Node *child = get_node_by_id(id);

		if (parent && child) {
			parent->add_child(child);
		}
	}

	m_path = path;
	return OK;
}

ErrorCode Scene::save(const char *path) {
	NodeDB &db = NodeDB::get();

	pb::Scene scn;
	scn.set_active_camera_2d(m_active_camera_2d);

	for (const auto &[id, count] : scene_resources().get_references()) {
		Resource *resource = ResourceManager::get().Get(id);

		pb::Resource *res = scn.add_resource();
		res->set_id(id);
		res->set_type(resource->Type());
		res->set_path(resource->Filepath());
	}

	scn.set_root_id(Root()->id());

	std::function<void(Node *, pb::Node *)> push;
	push = [&](Node *src, pb::Node *dst) {
		dst->set_id(src->id());
		dst->set_parent_id(src->get_parent() != nullptr ? src->get_parent()->id() : 0);
		dst->set_type(db.get_node_typename(src->type_hash()));
		dst->set_name(src->name());

		std::vector<std::string> props;
		db.get_property_names(src->type_hash(), props);
		for (const std::string &propName : props) {
			if (propName == "name")
				continue;

			NodeProperty prop = db.get_property(src->type_hash(), propName);

			const google::protobuf::Descriptor *descriptor = dst->GetDescriptor();
			const google::protobuf::FieldDescriptor *field = descriptor->FindFieldByName(propName);

			if (nullptr == field) {
				Utils::Error("Failed to find field {}", field->name());
				break;
			}

			switch (field->type()) {
			case google::protobuf::FieldDescriptor::TYPE_BOOL:
				dst->GetReflection()->SetBool(dst, field, std::any_cast<bool>(prop.get(src)));
				break;
			case google::protobuf::FieldDescriptor::TYPE_INT32:
				dst->GetReflection()->SetInt32(dst, field, std::any_cast<int>(prop.get(src)));
				break;
			case google::protobuf::FieldDescriptor::TYPE_FLOAT:
				dst->GetReflection()->SetFloat(dst, field, std::any_cast<float>(prop.get(src)));
				break;
			case google::protobuf::FieldDescriptor::TYPE_STRING: {
				std::string v = "";
				if (field->name() == "body_type") {
					v = PhysicsBodyTypeToString(std::any_cast<PhysicsBodyType>(prop.get(src)));
				} else {
					v = std::any_cast<std::string>(prop.get(src));
				}
				dst->GetReflection()->SetString(dst, field, v);
			} break;
			case google::protobuf::FieldDescriptor::TYPE_MESSAGE: {
				std::string msgName = field->message_type()->name();

				if (msgName == "Vec2") {
					pb::Vec2 *msg = new pb::Vec2;
					vec2 v = std::any_cast<vec2>(prop.get(src));
					msg->set_x(v.x);
					msg->set_y(v.y);

					dst->GetReflection()->SetAllocatedMessage(dst, msg, field);
				} else {
					Utils::Error("Unknown message type: {}", field->message_type()->name());
				}
			} break;

			default:
				Utils::Error("Unknown field type name: {}", field->type_name());
				break;
			}
		}

		for (auto &behaviour : src->get_behaviour_names()) {
			dst->add_behaviour(behaviour);
		}

		for (auto &group : src->get_groups()) {
			dst->add_group(group);
		}

		for (Node *child : src->get_children()) {
			pb::Node *c = scn.add_node();
			push(child, c);
		}
	};

	pb::Node *n = scn.add_node();
	push(Root(), n);

	Utils::Info("Saving scene to {}", path);

	std::string str;
	if (!google::protobuf::TextFormat::PrintToString(scn, &str)) {
		Utils::Error("Failed to generate scene file");
		return ERR_FAILED;
	}

	ErrorCode err = FileServer::get().save_file(str.data(), str.size(), path, ReadWriteFlags_AsText);
	if (err != OK)
		return err;

	m_path = path;
	return OK;
}

void Scene::clear() {
	Root() = nullptr;
	for (auto &[id, node] : m_allocated_nodes) {
		NodeDB::get().destruct(node);
	}
	m_allocated_nodes.clear();
}

Node *Scene::create(NodeType type, const std::string &name, size_t id) {
	Node *node = NodeDB::get().construct(type, name, id);
	while (m_allocated_nodes.find(node->m_id) != m_allocated_nodes.end()) {
		node->m_id = Utils::RandRange(1, 1'000'000);
	}

	node->m_scene = this;
	m_allocated_nodes[node->id()] = node;

	return node;
}

void Scene::queue_free(size_t id) {
	m_nodes_to_free.push_back(id);
}

void Scene::free(size_t id) {
	Node *node = get_node_by_id(id);
	if (nullptr == node)
		return;

	if (node == Root()) {
		Root() = nullptr;
	}

	if (Node *parent = node->get_parent(); nullptr != parent) {
		parent->remove_child(node->id());
	}
	for (Node *child : node->get_children()) {
		child->m_parent = nullptr;
	}

	for (size_t i = 0; i < node->get_children().size(); i++) {
		this->free(node->get_children()[i]->id());
	}

	NodeDB::get().destruct(node);

	m_allocated_nodes.erase(id);
}

Resource *Scene::load_resource(const std::string &path, RID id, ResourceType type) {
	Resource *res = ResourceManager::get().Load(path, id, type);
	if (nullptr != res) {
		m_resources.increment(res->ResourceID());
	}

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
	for (Node *node : Root()->get_children()) {
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
	dst->m_path = src->m_path;

	dst->m_resources.reset();
	for (const auto &[id, count] : src->m_resources.get_references()) {
		for (int i = 0; i < count; i++)
			dst->m_resources.increment(id);
	}

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

	dst->clear();
	Node *newRoot = src->Root()->duplicate_in_scene(dst);
	dst->Root() = newRoot;

	if (Camera2D *cam = src->get_active_camera2d(); nullptr != cam) {
		dst->set_active_camera2d(cam->id());
	}
}