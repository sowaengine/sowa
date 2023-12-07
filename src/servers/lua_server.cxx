#include "lua_server.hxx"

#include "sol/sol.hpp"

#include "core/behaviour/behaviour_db.hxx"
#include "scene/node_db.hxx"
#include "scene/nodes.hxx"
#include "servers/file_server.hxx"
#include "utils/utils.hxx"

#include <unordered_map>

struct LuaServerInternal {
	std::unordered_map<std::string, sol::table> behaviour_templates;

	std::unordered_map<uint32_t, sol::table> behaviour_instances;
};

static LuaServerInternal &internal(void *ptr) {
	return *reinterpret_cast<LuaServerInternal *>(ptr);
}

LuaServer &LuaServer::get() {
	static LuaServer *server = new LuaServer;
	server->p_internal = new LuaServerInternal;
	return *server;
}

void LuaServer::Init() {
	p_lua = new sol::state;
	p_lua->set_function("print", [](std::string e) { Utils::Log("{}", e); });

	p_lua->new_usertype<Node>(
		"Node",
		sol::meta_function::index, [](Node *node, sol::string_view key, sol::this_state state) {
		auto prop = NodeDB::get().get_property(node->type_hash(), std::string(key));
		if (prop.get_ref) {
			Property p = prop.get_ref(node);

			if (std::string **v = std::any_cast<std::string *>(&p); v != nullptr) {
				return sol::make_reference(state.lua_state(), **v);
			} else if (int **v = std::any_cast<int *>(&p); v != nullptr) {
				return sol::make_reference(state.lua_state(), **v);
			} else if (float **v = std::any_cast<float *>(&p); v != nullptr) {
				return sol::make_reference(state.lua_state(), **v);
			} else {
				Utils::Error("implement index metamethod for node property {}:{}", NodeDB::get().get_node_typename(node->type_hash()), key);
			}
		}

		return sol::make_reference(state.lua_state(), nullptr); },

		sol::meta_function::new_index, [](Node *node, sol::string_view key, sol::object value, sol::this_state state) {
		auto prop = NodeDB::get().get_property(node->type_hash(), std::string(key));
		if (prop.set) {
			if(value.get_type() == sol::type::number) {
				if(prop.typeName == "int") {
						prop.set(node, value.as<int>());
						return;
				} else if(prop.typeName == "float") {
						prop.set(node, value.as<float>());
						return;
				}
			} else if(value.get_type() == sol::type::string) {
				prop.set(node, value.as<std::string>());
				return;
			}
			Utils::Error("implement new_index metamethod for node property {}:{}", NodeDB::get().get_node_typename(node->type_hash()), key);
		}
			Utils::Error("failed to set invalid node property {}:{}", NodeDB::get().get_node_typename(node->type_hash()), key); });
}

void LuaServer::Load() {
	auto files = FileServer::get().read_dir("/scripts/", true);
	for (FileEntry &file : files) {
		if (file.Path().extension() != ".lua") {
			continue;
		}

		Result<file_buffer *> res = FileServer::get().load_file(file.Path().c_str(), ReadWriteFlags_AsText);
		if (!res.ok()) {
			Utils::Error("Failed to read {}. Error code: {}", file.Path().string(), (int)res.error());
			continue;
		}

		std::string script = std::string((char *)res.get()->data(), res.get()->size());
		auto behaviour_res = p_lua->do_string(script, file.Path(), sol::load_mode::text);
		if (behaviour_res.get_type() != sol::type::table) {
			Utils::Error("Behaviour file {} does not return table", file.Path().string());
			continue;
		}
		auto behaviour = behaviour_res.get<sol::table>();

		std::string name = behaviour["name"].get_or<std::string>(file.Path().filename().replace_extension(""));
		std::string extends = behaviour["extends"].get_or<std::string>("Node");

		//
		internal(p_internal).behaviour_templates[name] = behaviour;
		Utils::Info("Loaded behaviour with name {}", name);

		Behaviour b;
		b.SetExtends(extends);
		b.SetStartFunc([this, name](Node *node, Behaviour *self) {
			uint32_t behaviour_id = this->NewBehaviour(name);
			self->DataTable()["m_behaviour_id"] = behaviour_id;

			StartBehaviour(node, behaviour_id);
		});
		b.SetUpdateFunc([this](Node *node, Behaviour *self) {
			uint32_t *id = std::any_cast<uint32_t>(&self->DataTable()["m_behaviour_id"]);
			if (nullptr == id) {
				Utils::Error("m_behaviour_id field not found on behaviour {}", self->GetBehaviourName());
				return;
			}

			UpdateBehaviour(node, *id);
		});
		BehaviourDB::get().RegisterBehaviour(name, b);
	}
}

uint32_t LuaServer::NewBehaviour(const std::string &name) {
	auto &data = internal(p_internal);
	if (data.behaviour_templates.find(name) == data.behaviour_templates.end()) {
		return 0;
	}

	static uint32_t id_counter = 1;
	uint32_t id = id_counter++;

	sol::table &instance_table = data.behaviour_instances[id];
	sol::table &template_table = data.behaviour_templates[name];
	instance_table = p_lua->create_table();

	std::function<void(const sol::table &src, sol::table &dst)> copy_table;
	copy_table = [&](const sol::table &src, sol::table &dst) {
		for (sol::table::iterator it = src.begin(); it != src.end(); ++it) {
			sol::object key = (*it).first;
			sol::object value = (*it).second;

			if (value.get_type() == sol::type::table) {
				sol::table src = value.as<sol::table>();
				sol::table d = this->p_lua->create_table();
				copy_table(src, d);

				value = d;
			}

			dst[key] = value;
		}
	};

	copy_table(template_table, instance_table);

	return id;
}

void LuaServer::DestroyBehaviour(uint32_t id) {
	auto &data = internal(p_internal);
	if (id == 0 || data.behaviour_instances.find(id) == data.behaviour_instances.end()) {
		return;
	}

	data.behaviour_instances.erase(id);
}

void LuaServer::StartBehaviour(Node *node, uint32_t id) {
	auto &data = internal(p_internal);
	if (id == 0 || data.behaviour_instances.find(id) == data.behaviour_instances.end()) {
		return;
	}

	sol::table &behaviour = data.behaviour_instances[id];
	behaviour["node"] = sol::make_reference(p_lua->lua_state(), node);

	auto startFunc = behaviour["_start"];
	if (startFunc.get_type() == sol::type::function) {
		try {
			startFunc.operator()(behaviour);
		} catch (const std::exception &e) {
			Utils::Error("Failed to call _start(): {}", e.what());
		}
	}
}

void LuaServer::UpdateBehaviour(Node *node, uint32_t id) {
	auto &data = internal(p_internal);
	if (id == 0 || data.behaviour_instances.find(id) == data.behaviour_instances.end()) {
		return;
	}

	sol::table &behaviour = data.behaviour_instances[id];

	auto updateFunc = behaviour["_update"];
	if (updateFunc.get_type() == sol::type::function) {
		updateFunc.operator()(behaviour);
	}
}

LuaServer::LuaServer() {
}
LuaServer::~LuaServer() {
	delete reinterpret_cast<LuaServerInternal *>(p_internal);
}