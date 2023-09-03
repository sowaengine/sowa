#include "script_server.hxx"

#include <iostream>

#include "core/behaviour/behaviour_db.hxx"
#include "file_server.hxx"
#include "scene/node_2d.hxx"
#include "scene/node_db.hxx"
#include "scene/sprite_2d.hxx"

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

	return App::GetInstance().GetCurrentScene()->get_node_in_group("Barrel");
}
std::string get_name(Node *node) {
	return node->Name();
}
float get_pos_x(Node2D *node) {
	return 200.f;
}

ScriptServer &ScriptServer::GetInstance() {
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

	std::any prop = NodeDB::GetInstance().GetProperty(node->TypeHash(), propName).get(node);

	std::string typeName;
	void *ret = nullptr;

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
		ret = gen->GetEngine()->CreateScriptObject(propType);
	}

	if (typeName == "int") {
		if (int *v = std::any_cast<int>(&prop)) {
			gen->SetReturnDWord(*v);
			return;
		}
	} else if (typeName == "float") {
		if (float *v = std::any_cast<float>(&prop)) {
			gen->SetReturnFloat(*v);
			return;
		}
	} else if (typeName == "string") {
		if (std::string *v = std::any_cast<std::string>(&prop)) {
			*reinterpret_cast<std::string *>(ret) = *v;
			gen->SetReturnObject(ret);
			return;
		}
	}

	std::cout << "INVALID PROP" << std::endl;
}

static void asSetProperty(asIScriptGeneric *gen) {
	Node *node = reinterpret_cast<Node *>(gen->GetObject());

	asIScriptFunction *func = gen->GetFunction();
	std::string propName = std::string(func->GetName()).substr(4);

	NodeProperty prop = NodeDB::GetInstance().GetProperty(node->TypeHash(), propName);

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
	}

	std::cout << "INVALID PROP" << std::endl;
}

ScriptServer::ScriptServer() {
	s_data.engine = asCreateScriptEngine();

	AS_BEGIN();
	r = s_data.engine->SetMessageCallback(asFUNCTION(message_callback), 0, asCALL_CDECL);
	AS_CHECK();

	RegisterStdString(s_data.engine);

	AS_REGISTER_FUNC("void print(const string &in)", asFUNCTION(print), asCALL_CDECL);

	NodeDB &db = NodeDB::GetInstance();

	std::function<void(NodeType, NodeData)> registerInheritance;
	registerInheritance = [&](NodeType type, NodeData data) {
		if (data.extends == 0)
			return;

		std::string name = db.GetNodeTypeName(type);
		std::string parent = db.GetNodeTypeName(data.extends);

		AS(s_data.engine->RegisterObjectMethod(parent.c_str(), (name + "@ opCast()").c_str(), asFUNCTION(asOpCastParentToType), asCALL_GENERIC));
		AS_ASSERT();

		AS(s_data.engine->RegisterObjectMethod(name.c_str(), (parent + "@ opImplCast()").c_str(), asFUNCTION(asOpImplCastTypeToParent), asCALL_GENERIC));
		AS_ASSERT();
	};

	s_data.engine->RegisterTypedef("RID", "int");
	for (const auto &[type, data] : db.m_db) {
		std::string name = db.GetNodeTypeName(type);
		AS_REGISTER_TYPE_STR(name.c_str(), asOBJ_REF | asOBJ_NOCOUNT);
	}

	for (const auto &[type, data] : db.m_db) {
		std::string name = db.GetNodeTypeName(type);

		registerInheritance(type, data);
		for (const auto &[propName, prop] : data.properties) {
			std::string typeName = prop.typeName;

			if (typeName == "" || typeName == "glm::vec2")
				continue;

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

Error ScriptServer::LoadScriptFile(std::string path) {
	int r;

	std::string buf;
	Error err = FileServer::GetInstance().ReadFileString(path.c_str(), buf);
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

				BehaviourDB::GetInstance().RegisterBehaviour(t_info->GetName(), b);
			}
		}
	}
}