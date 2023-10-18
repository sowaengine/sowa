#include "script_server.hxx"

#include <iostream>

#include "core/behaviour/behaviour_db.hxx"
#include "core/time.hxx"

#include "file_server.hxx"

#include "math/math.hxx"

#include "scene/node_db.hxx"
#include "scene/nodes.hxx"

#include <angelscript.h>
#include <scriptbuilder/scriptbuilder.h>
#include <scriptstdstring/scriptstdstring.h>

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

	return App::get().GetCurrentScene()->get_node_in_group("Barrel");
}
std::string get_name(Node *node) {
	return node->name();
}
float get_pos_x(Node2D *node) {
	return 200.f;
}

ScriptServer &ScriptServer::get() {
	static ScriptServer *server = new ScriptServer;
	return *server;
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

ScriptServer::ScriptServer() {
	vec2 v;
	asConstruct<vec2>(&v, 0.f);
	asDestruct<vec2>(nullptr);

	s_data.engine = asCreateScriptEngine();

	AS_BEGIN();
	r = s_data.engine->SetMessageCallback(asFUNCTION(message_callback), 0, asCALL_CDECL);
	AS_CHECK();

	RegisterStdString(s_data.engine);

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
	s_data.engine->SetDefaultNamespace("");

	s_data.engine->SetDefaultNamespace("time");
	s_data.engine->RegisterGlobalFunction("double delta()", asFUNCTION(Time::Delta), asCALL_CDECL);
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

	struct SceneMethodWrapper {
		static Node *Create(const std::string &nodeType, const std::string &name) {
			return App::get().GetCurrentScene()->create(NodeDB::get().get_node_type(nodeType), name);
		}

		static Node *GetNodeInGroup(const std::string &group) {
			return App::get().GetCurrentScene()->get_node_in_group(group);
		}

		static Node *GetNodeById(uint64_t id) {
			return App::get().GetCurrentScene()->get_node_by_id(static_cast<size_t>(id));
		}

		static void QueueFree(Node *node) {
			return App::get().GetCurrentScene()->queue_free(node->id());
		}
	};

	s_data.engine->SetDefaultNamespace("scene");
	s_data.engine->RegisterGlobalFunction("Node@ create(string, string)", asFUNCTION(SceneMethodWrapper::Create), asCALL_CDECL);
	s_data.engine->RegisterGlobalFunction("Node@ get_node_in_group(string)", asFUNCTION(SceneMethodWrapper::GetNodeInGroup), asCALL_CDECL);
	s_data.engine->RegisterGlobalFunction("Node@ get_node_by_id(uint64)", asFUNCTION(SceneMethodWrapper::GetNodeById), asCALL_CDECL);
	s_data.engine->SetDefaultNamespace("");

	s_data.engine->RegisterObjectMethod("Node", "Node@ get_parent()", asMETHODPR(Node, get_parent, (), Node *), asCALL_THISCALL);
	s_data.engine->RegisterObjectMethod("Node", "Node@ add_child()", asMETHODPR(Node, add_child, (Node *), void), asCALL_THISCALL);
	s_data.engine->RegisterObjectMethod("Node", "void queue_free()", asFUNCTION(SceneMethodWrapper::QueueFree), asCALL_CDECL_OBJFIRST);
	s_data.engine->RegisterObjectMethod("Node2D", "float global_rotation()", asMETHODPR(Node2D, global_rotation, (), float), asCALL_THISCALL);

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
}

ScriptServer::~ScriptServer() {
	int r = s_data.engine->ShutDownAndRelease();
	AS_CHECK();
}

void ScriptServer::BeginBuild() {
	int r = s_data.builder.StartNewModule(s_data.engine, "Sowa");
	AS_CHECK();
}

ErrorCode ScriptServer::LoadScriptFile(std::string path) {
	int r;

	std::string buf;
	ErrorCode err = FileServer::get().ReadFileString(path.c_str(), buf);
	if (err != OK)
		return err;

	r = s_data.builder.AddSectionFromMemory(path.c_str(), buf.c_str());
	AS_CHECK();

	return OK;
}

void ScriptServer::EndBuild() {
	register_script_behaviour();
	int r = s_data.builder.BuildModule();
	AS_CHECK();

	register_script_behaviour();
}

void ScriptServer::register_script_behaviour() {
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
}