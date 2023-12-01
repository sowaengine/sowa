#include "script_server.hxx"

#include <iostream>

#include "core/app.hxx"
#include "core/behaviour/behaviour_db.hxx"
#include "core/store.hxx"
#include "core/time.hxx"
#include "core/tweens.hxx"
#include "resource/resource_manager.hxx"

#include "file_server.hxx"

#include "math/math.hxx"

#include "scene/node_db.hxx"
#include "scene/nodes.hxx"
#include "scene/scene.hxx"
#include "scene/scene_tree.hxx"

#ifndef SW_WEB
#include <angelscript.h>
#include <scriptarray/scriptarray.h>
#include <scriptbuilder/scriptbuilder.h>
#include <scriptstdstring/scriptstdstring.h>
#endif

#ifdef GetObject
#undef GetObject
#endif

#ifndef SW_WEB

#define AS_CHECK()                                                        \
	do {                                                                  \
		if (r < 0) {                                                      \
			std::cout << "ScriptServer Error: " << __LINE__ << std::endl; \
		}                                                                 \
	} while (0);

#define AS_BEGIN() int r

#define AS(f)                                                             \
	do {                                                                  \
		r = f;                                                            \
		if (r < 0) {                                                      \
			std::cout << "ScriptServer Error: " << __LINE__ << std::endl; \
		}                                                                 \
                                                                          \
	} while (0)

#define AS_ASSERT()  \
	do {             \
		if (r < 0) { \
			return;  \
		}            \
	} while (0)

struct ScriptServerData {
	asIScriptEngine *engine;

	CScriptBuilder builder;
};

static ScriptServerData s_data;

static int s_alloc = 0;
static int s_dealloc = 0;

namespace ASRef {
// ref -1: app allocated object
static std::unordered_map<void *, int> refs;

template <typename T>
void AddRef(T *o) {
	if (refs[(void *)o] == -1) {
		return;
	}

	refs[(void *)o]++;
}

template <typename T>
void Release(T *o) {

	if (refs.find((void *)o) == refs.end()) {
		return;
	}

	if (refs[(void *)o] == -1) {
		return;
	}

	if (--refs[(void *)o] == 0) {
		s_dealloc += sizeof(T);
		refs.erase(refs.find((void *)o));

		delete o;

	} else {
		Utils::Error("Checkout {}:{}", __FILE__, __LINE__);
	}
}

template <typename T, typename... Args>
T *Create(Args... args) {

	T *t = new T(args...);
	refs[(void *)t] = 1;

	s_alloc += sizeof(T);

	// Utils::Log("Alloc: {}, Dealloc: {}, Usage: {}", s_alloc, s_dealloc, s_alloc - s_dealloc);
	return t;
}

// Stores script object without allocating it. (used for app allocated objects)
template <typename T>
T *CreateNoAlloc(T *o) {
	refs[(void *)o] = -1;
	return o;
}
} // namespace ASRef

// Implement a simple message callback function
void message_callback(const asSMessageInfo *msg, void *param) {
	const char *type = "ERR ";
	if (msg->type == asMSGTYPE_WARNING)
		type = "WARN";
	else if (msg->type == asMSGTYPE_INFORMATION)
		type = "INFO";
	printf("%s (%d, %d) : %s : %s\n", msg->section, msg->row, msg->col, type, msg->message);
}

void print(std::string &msg) {
	printf("%s\n", msg.c_str());
}

Node *get() {
	int a = 1;

	return SceneTree::get().get_scene()->get_node_in_group("Barrel");
}
std::string get_name(Node *node) {
	return node->name();
}
float get_pos_x(Node2D *node) {
	return 200.f;
}

// Example opCast behaviour
template <class A, class B>
B *refCast(A *a) {
	// If the handle already is a null handle, then just return the null handle
	if (!a)
		return 0;

	// Now try to dynamically cast the pointer to the wanted type
	B *b = dynamic_cast<B *>(a);
	if (b != 0) {
		// Since the cast was made, we need to increase the ref counter for the returned handle
		// b->addref();
	}
	return b;
}

#define AS_REGISTER_TYPE_STR(type, flags)                             \
	AS(s_data.engine->RegisterObjectType(type, sizeof(type), flags)); \
	AS_ASSERT()

#define AS_REGISTER_TYPE(type, flags) AS_REGISTER_TYPE_STR(#type, flags)

#define AS_REGISTER_METHOD_STR(type, decl, func, callConv)               \
	AS(s_data.engine->RegisterObjectMethod(type, decl, func, callConv)); \
	AS_ASSERT()

#define AS_REGISTER_METHOD(type, decl, func, callConv) AS_REGISTER_METHOD_STR(#type, decl, func, callConv)

#define AS_REGISTER_FUNC(decl, func, callConv)                       \
	AS(s_data.engine->RegisterGlobalFunction(decl, func, callConv)); \
	AS_ASSERT()

#define AS_REGISTER_INHERITANCE(type, parent)                                                                                            \
	AS(s_data.engine->RegisterObjectMethod(#parent, #type "@ opCast()", asFUNCTION((refCast<parent, type>)), asCALL_CDECL_OBJLAST));     \
	AS_ASSERT();                                                                                                                         \
	AS(s_data.engine->RegisterObjectMethod(#type, #parent "@ opImplCast()", asFUNCTION((refCast<type, parent>)), asCALL_CDECL_OBJLAST)); \
	AS_ASSERT()

static void asOpCastParentToType(asIScriptGeneric *gen) {
	int parentID = gen->GetObjectTypeId();
	asITypeInfo *parentType = gen->GetEngine()->GetTypeInfoById(parentID);

	int retID = gen->GetReturnTypeId();
	asITypeInfo *retType = gen->GetEngine()->GetTypeInfoById(retID);

	Node *node = reinterpret_cast<Node *>(gen->GetObject());
	gen->SetReturnObject(node);
}

static void asOpImplCastTypeToParent(asIScriptGeneric *gen) {
	int parentID = gen->GetReturnTypeId();
	asITypeInfo *parentType = gen->GetEngine()->GetTypeInfoById(parentID);

	int retID = gen->GetObjectTypeId();
	asITypeInfo *retType = gen->GetEngine()->GetTypeInfoById(retID);

	Node *node = reinterpret_cast<Node *>(gen->GetObject());
	gen->SetReturnObject(node);
}

static void asGetProperty(asIScriptGeneric *gen) {
	Node *node = reinterpret_cast<Node *>(gen->GetObject());

	asIScriptFunction *func = gen->GetFunction();
	std::string propName = std::string(func->GetName()).substr(4);

	std::string typeName;
	// void *ret = nullptr;

	int propTypeID = gen->GetReturnTypeId();
	if (propTypeID == asTYPEID_INT32) {
		typeName = "int";
	} else if (propTypeID == asTYPEID_FLOAT) {
		typeName = "float";
	} else if (propTypeID == asTYPEID_BOOL) {
		typeName = "bool";
	} else {
		asITypeInfo *propType = gen->GetEngine()->GetTypeInfoById(propTypeID);
		if (nullptr == propType) {
			std::cout << "Invalid error: " << __FILE__ << " : " << __LINE__ << std::endl;
			return;
		}

		typeName = propType->GetName();
		// ret = gen->GetEngine()->CreateScriptObject(propType);
	}

	if (typeName == "int") {
		std::any prop = NodeDB::get().get_property(node->type_hash(), propName).get(node);
		if (int *v = std::any_cast<int>(&prop)) {
			gen->SetReturnDWord(*v);
			return;
		}
	} else if (typeName == "float") {
		std::any prop = NodeDB::get().get_property(node->type_hash(), propName).get(node);
		if (float *v = std::any_cast<float>(&prop)) {
			gen->SetReturnFloat(*v);
			return;
		}
	} else if (typeName == "bool") {
		std::any prop = NodeDB::get().get_property(node->type_hash(), propName).get(node);
		if (bool *v = std::any_cast<bool>(&prop)) {
			gen->SetReturnByte(*v ? 1 : 0);
			return;
		}
	} else if (typeName == "string") {
		std::any prop = NodeDB::get().get_property(node->type_hash(), propName).get_ref(node);
		if (std::string **v = std::any_cast<std::string *>(&prop)) {
			gen->SetReturnObject(ASRef::CreateNoAlloc(*v));
			return;
		}
	} else if (typeName == "vec2") {
		std::any prop = NodeDB::get().get_property(node->type_hash(), propName).get_ref(node);
		if (vec2 **v = std::any_cast<vec2 *>(&prop)) {
			gen->SetReturnObject(ASRef::CreateNoAlloc(*v));
			return;
		}
	}

	std::cout << "INVALID PROP" << std::endl;
}

static void asSetProperty(asIScriptGeneric *gen) {
	Node *node = reinterpret_cast<Node *>(gen->GetObject());

	asIScriptFunction *func = gen->GetFunction();
	std::string propName = std::string(func->GetName()).substr(4);

	NodeProperty prop = NodeDB::get().get_property(node->type_hash(), propName);

	std::string typeName;

	int propTypeID = gen->GetArgTypeId(0);
	if (propTypeID == asTYPEID_INT32) {
		typeName = "int";
	} else if (propTypeID == asTYPEID_FLOAT) {
		typeName = "float";
	} else if (propTypeID == asTYPEID_BOOL) {
		typeName = "bool";
	} else {
		asITypeInfo *propType = gen->GetEngine()->GetTypeInfoById(propTypeID);
		if (nullptr == propType) {
			std::cout << "Invalid error: " << __FILE__ << " : " << __LINE__ << std::endl;
			return;
		}

		typeName = propType->GetName();
	}

	if (typeName == "int") {
		int arg = gen->GetArgDWord(0);
		prop.set(node, arg);
		return;

	} else if (typeName == "float") {
		float arg = gen->GetArgFloat(0);
		prop.set(node, arg);
		return;

	} else if (typeName == "bool") {
		bool arg = gen->GetArgByte(0) == 1;
		prop.set(node, arg);
		return;

	} else if (typeName == "string") {
		std::string *v = reinterpret_cast<std::string *>(gen->GetArgObject(0));
		prop.set(node, *v);
		return;
	} else if (typeName == "vec2") {
		vec2 *v = reinterpret_cast<vec2 *>(gen->GetArgObject(0));
		prop.set(node, *v);
		return;
	}

	std::cout << "INVALID PROP" << std::endl;
}

template <typename T>
void asConstruct(void *memory) {
	new (memory) T();
}

template <typename T>
void asConstruct(void *memory, float v) {
	new (memory) T(v);
}

template <typename T>
void asConstruct(void *memory, float v1, float v2) {
	new (memory) T(v1, v2);
}

template <typename T>
void asConstruct(void *memory, const vec2 &v) {
	new (memory) T(v);
}

template <typename T>
void asDestruct(void *memory) {
	((T *)memory)->~T();
}
#endif

ScriptServer &ScriptServer::get() {
	static ScriptServer *server = new ScriptServer;
	return *server;
}

ScriptServer::ScriptServer() {
#ifndef SW_WEB

	vec2 v;
	asConstruct<vec2>(&v, 0.f);
	asDestruct<vec2>(nullptr);

	s_data.engine = asCreateScriptEngine();

	AS_BEGIN();
	r = s_data.engine->SetMessageCallback(asFUNCTION(message_callback), 0, asCALL_CDECL);
	AS_CHECK();

	RegisterStdString(s_data.engine);
	RegisterScriptArray(s_data.engine, true);

	AS_REGISTER_FUNC("void print(const string &in)", asFUNCTION(print), asCALL_CDECL);

	NodeDB &db = NodeDB::get();

	std::function<void(NodeType, NodeData)> registerInheritance;
	registerInheritance = [&](NodeType type, NodeData data) {
		if (data.extends == 0)
			return;

		std::string name = db.get_node_typename(type);
		std::string parent = db.get_node_typename(data.extends);

		AS(s_data.engine->RegisterObjectMethod(parent.c_str(), (name + "@ opCast()").c_str(), asFUNCTION(asOpCastParentToType), asCALL_GENERIC));
		AS_ASSERT();

		AS(s_data.engine->RegisterObjectMethod(name.c_str(), (parent + "@ opImplCast()").c_str(), asFUNCTION(asOpImplCastTypeToParent), asCALL_GENERIC));
		AS_ASSERT();
	};

	// Register types
	s_data.engine->RegisterTypedef("RID", "int");

	s_data.engine->RegisterEnum("PhysicsBodyType");
	s_data.engine->RegisterEnumValue("PhysicsBodyType", "Static", (int)PhysicsBodyType::Static);
	s_data.engine->RegisterEnumValue("PhysicsBodyType", "Dynamic", (int)PhysicsBodyType::Dynamic);
	s_data.engine->RegisterEnumValue("PhysicsBodyType", "Kinematic", (int)PhysicsBodyType::Kinematic);

	s_data.engine->RegisterObjectType("vec2", sizeof(vec2), asOBJ_REF);
	s_data.engine->RegisterObjectBehaviour("vec2", asBEHAVE_ADDREF, "void f()", asFUNCTION(ASRef::AddRef<vec2>), asCALL_CDECL_OBJFIRST);
	s_data.engine->RegisterObjectBehaviour("vec2", asBEHAVE_RELEASE, "void f()", asFUNCTION(ASRef::Release<vec2>), asCALL_CDECL_OBJFIRST);

	s_data.engine->RegisterObjectBehaviour("vec2", asBEHAVE_FACTORY, "vec2@ f()", asFUNCTIONPR(ASRef::Create<vec2>, (), vec2 *), asCALL_CDECL);
	s_data.engine->RegisterObjectBehaviour("vec2", asBEHAVE_FACTORY, "vec2@ f(float)", asFUNCTIONPR(ASRef::Create<vec2>, (float), vec2 *), asCALL_CDECL);
	s_data.engine->RegisterObjectBehaviour("vec2", asBEHAVE_FACTORY, "vec2@ f(float, float)", asFUNCTIONPR(ASRef::Create<vec2>, (float, float), vec2 *), asCALL_CDECL);
	s_data.engine->RegisterObjectMethod("vec2", "vec2 &opAssign(vec2 &in) const", asMETHODPR(vec2, operator=, (const vec2 &), vec2 &), asCALL_THISCALL);
	s_data.engine->RegisterObjectProperty("vec2", "float x", asOFFSET(vec2, x));
	s_data.engine->RegisterObjectProperty("vec2", "float y", asOFFSET(vec2, y));

	s_data.engine->SetDefaultNamespace("math");
	s_data.engine->RegisterGlobalFunction("float sin(float)", asFUNCTION(math::sin), asCALL_CDECL);
	s_data.engine->RegisterGlobalFunction("float cos(float)", asFUNCTION(math::cos), asCALL_CDECL);
	s_data.engine->RegisterGlobalFunction("float pi()", asFUNCTION(math::pi), asCALL_CDECL);
	s_data.engine->RegisterGlobalFunction("float radians(float)", asFUNCTION(math::radians), asCALL_CDECL);
	s_data.engine->RegisterGlobalFunction("float degrees(float)", asFUNCTION(math::degrees), asCALL_CDECL);
	s_data.engine->RegisterGlobalFunction("float atan2(float, float)", asFUNCTION(math::atan2), asCALL_CDECL);
	s_data.engine->RegisterGlobalFunction("float fmod(float, float)", asFUNCTION(math::fmod), asCALL_CDECL);
	s_data.engine->RegisterGlobalFunction("float sqrt(float)", asFUNCTION(math::sqrt), asCALL_CDECL);
	s_data.engine->SetDefaultNamespace("");

	s_data.engine->SetDefaultNamespace("time");
	s_data.engine->RegisterGlobalFunction("double delta()", asFUNCTION(Time::Delta), asCALL_CDECL);
	s_data.engine->SetDefaultNamespace("");

	struct UtilsMethodWrapper {
		static void Log(const std::string &msg) {
			Utils::Log(msg);
		}

		static void Info(const std::string &msg) {
			Utils::Info(msg);
		}

		static void Error(const std::string &msg) {
			Utils::Error(msg);
		}

		static void Warn(const std::string &msg) {
			Utils::Warn(msg);
		}

		static int RandRange(int min, int max) {
			return Utils::RandRange(min, max);
		}
	};

	s_data.engine->SetDefaultNamespace("utils");
	s_data.engine->RegisterGlobalFunction("void log(string)", asFUNCTION(UtilsMethodWrapper::Log), asCALL_CDECL);
	s_data.engine->RegisterGlobalFunction("void info(string)", asFUNCTION(UtilsMethodWrapper::Info), asCALL_CDECL);
	s_data.engine->RegisterGlobalFunction("void error(string)", asFUNCTION(UtilsMethodWrapper::Error), asCALL_CDECL);
	s_data.engine->RegisterGlobalFunction("void warn(string)", asFUNCTION(UtilsMethodWrapper::Warn), asCALL_CDECL);
	s_data.engine->RegisterGlobalFunction("int rand_range(int, int)", asFUNCTION(UtilsMethodWrapper::RandRange), asCALL_CDECL);
	s_data.engine->SetDefaultNamespace("");

	struct InputMethodWrapper {
		static vec2 *GetGameMousePosition() {
			return ASRef::Create<vec2>(Input::GetGameMousePosition());
		}
	};

	s_data.engine->SetDefaultNamespace("input");
	s_data.engine->RegisterGlobalFunction("vec2@ get_game_mouse_position()", asFUNCTION(InputMethodWrapper::GetGameMousePosition), asCALL_CDECL);
	s_data.engine->SetDefaultNamespace("");

	// Register nodes
	for (const auto &[type, data] : db.m_db) {
		std::string name = db.get_node_typename(type);
		AS_REGISTER_TYPE_STR(name.c_str(), asOBJ_REF | asOBJ_NOCOUNT);
	}

	// TODO: Temporary
	struct GlobalAudioMethodWrapper {
		static AudioStreamPlayer *get() {
			static AudioStreamPlayer *player = new AudioStreamPlayer;
			return ASRef::CreateNoAlloc<AudioStreamPlayer>(player);
		}

		static void LoadAudio(const std::string &path) {
			Resource *res = ResourceManager::get().Load(path, 0, ResourceType_AudioStream);
			if (res == nullptr) {
				return;
			}

			get()->stop();
			get()->stream() = res->ResourceID();
		}
	};

	s_data.engine->SetDefaultNamespace("global_audio");
	s_data.engine->RegisterGlobalFunction("AudioStreamPlayer@ get()", asFUNCTION(GlobalAudioMethodWrapper::get), asCALL_CDECL);
	s_data.engine->RegisterGlobalFunction("void load_audio(string)", asFUNCTION(GlobalAudioMethodWrapper::LoadAudio), asCALL_CDECL);
	s_data.engine->SetDefaultNamespace("");

	struct SceneMethodWrapper {
		static Node *Create(const std::string &nodeType, const std::string &name) {
			return SceneTree::get().get_scene()->create(NodeDB::get().get_node_type(nodeType), name);
		}

		static Node *GetNodeInGroup(const std::string &group) {
			return SceneTree::get().get_scene()->get_node_in_group(group);
		}

		static Node *GetNodeById(uint64_t id) {
			return SceneTree::get().get_scene()->get_node_by_id(static_cast<size_t>(id));
		}

		static Node *Root() {
			return SceneTree::get().get_scene()->Root();
		}

		static void QueueFree(Node *node) {
			return SceneTree::get().get_scene()->queue_free(node->id());
		}
	};

	s_data.engine->SetDefaultNamespace("scene");
	s_data.engine->RegisterGlobalFunction("Node@ create(string, string)", asFUNCTION(SceneMethodWrapper::Create), asCALL_CDECL);
	s_data.engine->RegisterGlobalFunction("Node@ get_node_in_group(string)", asFUNCTION(SceneMethodWrapper::GetNodeInGroup), asCALL_CDECL);
	s_data.engine->RegisterGlobalFunction("Node@ get_node_by_id(uint64)", asFUNCTION(SceneMethodWrapper::GetNodeById), asCALL_CDECL);
	s_data.engine->RegisterGlobalFunction("Node@ root()", asFUNCTION(SceneMethodWrapper::Root), asCALL_CDECL);
	s_data.engine->SetDefaultNamespace("");

	struct AppMethodWrapper {
		static void LoadScene(const std::string &path) {
			App::get().load_scene(path);
		}

		static void LoadSceneDelayed(const std::string &path, float delay) {
			Tweens::get().RegisterTween(
				delay, [path](float v) {}, [path]() { App::get().load_scene(path); },
				Utils::Easing::LINEAR);
		}
	};

	s_data.engine->SetDefaultNamespace("app");
	s_data.engine->RegisterGlobalFunction("void load_scene(string)", asFUNCTION(AppMethodWrapper::LoadScene), asCALL_CDECL);
	s_data.engine->RegisterGlobalFunction("void load_scene_delayed(string, float)", asFUNCTION(AppMethodWrapper::LoadSceneDelayed), asCALL_CDECL);
	s_data.engine->SetDefaultNamespace("");

	struct PhysicsMethodWrapper {
		static void SetGravity(float x, float y) {
			PhysicsServer2D::get().set_gravity(vec2(x, y));
		}
	};

	s_data.engine->SetDefaultNamespace("physics");
	s_data.engine->RegisterGlobalFunction("void set_gravity(float, float)", asFUNCTION(PhysicsMethodWrapper::SetGravity), asCALL_CDECL);
	s_data.engine->SetDefaultNamespace("");

	struct StoreMethodWrapper {
		using store_type = Store<std::string, std::string>;

		static std::string GetValue(const std::string &key) {
			return store_type::get().get_value(key);
		}
		static void SetValue(const std::string &key, const std::string &value) {
			store_type::get().set_value(key, value);
		}

		static bool HasKey(const std::string &key) {
			return store_type::get().has_key(key);
		}

		static void RemoveKey(const std::string &key) {
			store_type::get().remove_key(key);
		}

		static void Clear() {
			store_type::get().clear();
		}
	};

	s_data.engine->SetDefaultNamespace("store");
	s_data.engine->RegisterGlobalFunction("string get_value(string)", asFUNCTION(StoreMethodWrapper::GetValue), asCALL_CDECL);
	s_data.engine->RegisterGlobalFunction("void set_value(string, string)", asFUNCTION(StoreMethodWrapper::SetValue), asCALL_CDECL);
	s_data.engine->RegisterGlobalFunction("bool has_key(string)", asFUNCTION(StoreMethodWrapper::HasKey), asCALL_CDECL);
	s_data.engine->RegisterGlobalFunction("void remove_key(string)", asFUNCTION(StoreMethodWrapper::RemoveKey), asCALL_CDECL);
	s_data.engine->RegisterGlobalFunction("void clear()", asFUNCTION(StoreMethodWrapper::Clear), asCALL_CDECL);
	s_data.engine->SetDefaultNamespace("");

	s_data.engine->RegisterEnum("Key");
	s_data.engine->RegisterEnumValue("Key", "Space", KEY_SPACE);
	s_data.engine->RegisterEnumValue("Key", "W", KEY_W);
	s_data.engine->RegisterEnumValue("Key", "A", KEY_A);
	s_data.engine->RegisterEnumValue("Key", "S", KEY_S);
	s_data.engine->RegisterEnumValue("Key", "D", KEY_D);
	s_data.engine->RegisterEnumValue("Key", "R", KEY_R);
	s_data.engine->SetDefaultNamespace("input");
	s_data.engine->RegisterGlobalFunction("bool is_key_down(int)", asFUNCTION(Input::IsKeyDown), asCALL_CDECL);
	s_data.engine->RegisterGlobalFunction("bool is_key_just_pressed(int)", asFUNCTION(Input::IsKeyJustPressed), asCALL_CDECL);
	s_data.engine->SetDefaultNamespace("");

	s_data.engine->RegisterObjectMethod("Node", "Node@ get_parent()", asMETHODPR(Node, get_parent, (), Node *), asCALL_THISCALL);
	s_data.engine->RegisterObjectMethod("Node", "void add_child(Node@)", asMETHODPR(Node, add_child, (Node *), void), asCALL_THISCALL);
	s_data.engine->RegisterObjectMethod("Node", "void add_group(string)", asMETHODPR(Node, add_group, (const std::string &), void), asCALL_THISCALL);
	s_data.engine->RegisterObjectMethod("Node", "uint64 get_id()", asMETHODPR(Node, id, (), size_t), asCALL_THISCALL);
	s_data.engine->RegisterObjectMethod("Node", "Node@ duplicate()", asMETHODPR(Node, duplicate, (), Node *), asCALL_THISCALL);
	s_data.engine->RegisterObjectMethod("Node", "bool is_in_group(string)", asMETHODPR(Node, is_in_group, (const std::string &), bool), asCALL_THISCALL);
	s_data.engine->RegisterObjectMethod("Node", "int64 get_child_count()", asMETHODPR(Node, get_child_count, (), size_t), asCALL_THISCALL);
	s_data.engine->RegisterObjectMethod("Node", "void queue_free()", asFUNCTION(SceneMethodWrapper::QueueFree), asCALL_CDECL_OBJFIRST);
	s_data.engine->RegisterObjectMethod("Node2D", "float global_rotation()", asMETHODPR(Node2D, global_rotation, (), float), asCALL_THISCALL);
	s_data.engine->RegisterObjectMethod("AudioStreamPlayer", "void play()", asMETHODPR(AudioStreamPlayer, play, (), void), asCALL_THISCALL);
	s_data.engine->RegisterObjectMethod("AudioStreamPlayer", "bool is_playing()", asMETHODPR(AudioStreamPlayer, is_playing, (), bool), asCALL_THISCALL);
	s_data.engine->RegisterObjectMethod("PhysicsBody2D", "void set_linear_velocity(vec2@)", asMETHODPR(PhysicsBody2D, set_linear_velocity, (vec2), void), asCALL_THISCALL);

	for (const auto &[type, data] : db.m_db) {
		std::string name = db.get_node_typename(type);

		registerInheritance(type, data);
		for (const auto &[propName, prop] : data.properties) {
			std::string typeName = prop.typeName;

			if (typeName == "")
				continue;

			if (typeName == "vec2")
				typeName += "@";

			if (typeName == "std::string")
				typeName = "string";

			AS_REGISTER_METHOD_STR(name.c_str(), (typeName + " get_" + propName + "() property").c_str(), asFUNCTION(asGetProperty), asCALL_GENERIC);
			AS_REGISTER_METHOD_STR(name.c_str(), ("void set_" + propName + "(" + typeName + ") property").c_str(), asFUNCTION(asSetProperty), asCALL_GENERIC);
		}
	}
#endif
}

ScriptServer::~ScriptServer() {
#ifndef SW_WEB

	int r = s_data.engine->ShutDownAndRelease();
	AS_CHECK();

#endif
}

void ScriptServer::BeginBuild() {
#ifndef SW_WEB

	int r = s_data.builder.StartNewModule(s_data.engine, "Sowa");
	AS_CHECK();

#endif
}

ErrorCode ScriptServer::LoadScriptFile(std::string path) {
#ifndef SW_WEB
	/*
	int r;

	std::string buf;
	ErrorCode err = FileServer::get().ReadFileString(path.c_str(), buf);
	if (err != OK)
		return err;

	r = s_data.builder.AddSectionFromMemory(path.c_str(), buf.c_str());
	AS_CHECK();
	*/

#endif
	return OK;
}

void ScriptServer::EndBuild() {
#ifndef SW_WEB
	int r = s_data.builder.BuildModule();
	AS_CHECK();

	register_script_behaviour();
#endif
}

void ScriptServer::register_script_behaviour() {
#ifndef SW_WEB
	asUINT c = s_data.engine->GetModule("Sowa")->GetObjectTypeCount();
	for (asUINT i = 0; i < c; i++) {
		asITypeInfo *t_info = s_data.engine->GetModule("Sowa")->GetObjectTypeByIndex(i);
		int typeID = t_info->GetTypeId();

		auto tags = s_data.builder.GetMetadataForType(t_info->GetTypeId());
		for (const std::string &tag : tags) {
			if (tag == "behaviour") {

				Behaviour b;
				b.SetStartFunc([typeID](Node *node, Behaviour *self) {
					asITypeInfo *t_info = s_data.engine->GetTypeInfoById(typeID);
					asIScriptObject *obj = (asIScriptObject *)s_data.engine->CreateScriptObject(t_info);
					self->DataTable()["m_obj"] = obj;

					bool *o = std::any_cast<bool>(&self->DataTable()["m_noStart"]);
					if (nullptr != o && *o == true) {
						return;
					}

					asIScriptFunction *func = obj->GetObjectType()->GetMethodByName("_start");
					if (func == 0) {
						return;
					}
					asIScriptContext *ctx = s_data.engine->CreateContext();
					int r = ctx->Prepare(func);
					AS_CHECK();

					r = ctx->SetArgObject(0, node);
					AS_CHECK();

					r = ctx->SetObject(obj);
					AS_CHECK();

					r = ctx->Execute();
					if (r != asEXECUTION_FINISHED) {
						// The execution didn't complete as expected. Determine what happened.
						if (r == asEXECUTION_EXCEPTION) {
							// An exception occurred, let the script writer know what happened so it can be corrected.
							printf("An exception '%s' occurred. Please correct the code and try again.\n", ctx->GetExceptionString());
						}
					}
					ctx->Release();
				});

				b.SetUpdateFunc([](Node *node, Behaviour *self) {
					asIScriptObject **o = std::any_cast<asIScriptObject *>(&self->DataTable()["m_obj"]);
					if (!o) {
						return;
					}

					asIScriptObject *obj = *o;

					asIScriptFunction *func = obj->GetObjectType()->GetMethodByName("_update");
					if (func == 0) {
						return;
					}
					asIScriptContext *ctx = s_data.engine->CreateContext();
					int r = ctx->Prepare(func);
					AS_CHECK();

					r = ctx->SetArgObject(0, node);
					AS_CHECK();

					r = ctx->SetObject(obj);
					AS_CHECK();

					r = ctx->Execute();
					if (r != asEXECUTION_FINISHED) {
						// The execution didn't complete as expected. Determine what happened.
						if (r == asEXECUTION_EXCEPTION) {
							// An exception occurred, let the script writer know what happened so it can be corrected.
							printf("An exception '%s' occurred. Please correct the code and try again.\n", ctx->GetExceptionString());
						}
					}
					ctx->Release();
				});

				BehaviourDB::get().RegisterBehaviour(t_info->GetName(), b);
			}
		}
	}
#endif
}

ScriptFunctionCaller::ScriptFunctionCaller() {
#ifndef SW_WEB
	m_arg_counter = 0;
	ctx = nullptr;
#endif
}

ScriptFunctionCaller::ScriptFunctionCaller(const std::string &moduleName, const std::string className, const std::string &decl) {
#ifndef SW_WEB
	m_arg_counter = 0;
	ctx = nullptr;

	asIScriptModule *module_ = s_data.engine->GetModule(moduleName.c_str());
	if (nullptr == module_) {
		// Utils::Error("Invalid module");
		return;
	}

	asITypeInfo *typeinfo = module_->GetTypeInfoByDecl(className.c_str());
	if (nullptr == typeinfo) {
		// Utils::Error("Invalid type");
		return;
	}

	asIScriptFunction *func = typeinfo->GetMethodByDecl(decl.c_str());
	if (nullptr == func) {
		// Utils::Error("Invalid function");
		return;
	}

	ctx = s_data.engine->CreateContext();
	ctx->Prepare(func);
#endif
}

ScriptFunctionCaller::ScriptFunctionCaller(const std::string &moduleName, const std::string &decl) {
#ifndef SW_WEB
	m_arg_counter = 0;
	ctx = nullptr;

	ctx = s_data.engine->CreateContext();
	asIScriptFunction *func = s_data.engine->GetModule(moduleName.c_str())->GetFunctionByDecl(decl.c_str());
	ctx->Prepare(func);
#endif
}

ScriptFunctionCaller &ScriptFunctionCaller::arg() {
#ifndef SW_WEB
	if (nullptr == ctx) {
		return *this;
	}

	m_arg_counter++;
#endif
	return *this;
}

ScriptFunctionCaller &ScriptFunctionCaller::arg_this(void *o) {
#ifndef SW_WEB
	if (nullptr == ctx) {
		return *this;
	}

	ctx->SetObject(o);
#endif
	return *this;
}

ScriptFunctionCaller &ScriptFunctionCaller::arg_object(void *o) {
#ifndef SW_WEB
	if (nullptr == ctx) {
		return *this;
	}

	ctx->SetArgObject(m_arg_counter++, o);
#endif
	return *this;
}

ScriptFunctionCaller &ScriptFunctionCaller::arg_u32(uint32_t value) {
#ifndef SW_WEB
	if (nullptr == ctx) {
		return *this;
	}

	ctx->SetArgDWord(m_arg_counter++, value);
#endif
	return *this;
}

ScriptFunctionCaller &ScriptFunctionCaller::arg_u64(uint64_t value) {
#ifndef SW_WEB
	if (nullptr == ctx) {
		return *this;
	}

	ctx->SetArgQWord(m_arg_counter++, value);
#endif
	return *this;
}

void ScriptFunctionCaller::call() {
#ifndef SW_WEB
	if (nullptr == ctx) {
		Utils::Error("Attempted to call a script function with invalid context");
		return;
	}

	int r = ctx->Execute();
	if (r == asEXECUTION_FINISHED) {
		// The return value is only valid if the execution finished successfully
		// asDWORD ret = ctx->GetReturnDWord();
	} else {
		Utils::Error("script execution error: {}", r);
	}

	ctx->Release();
#endif
}

ScriptFunctionCaller::~ScriptFunctionCaller() {
}