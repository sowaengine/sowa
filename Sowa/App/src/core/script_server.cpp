#include "script_server.hpp"

#include "add_on/scriptbuilder/scriptbuilder.h"
#include "add_on/scriptstdstring/scriptstdstring.h"
#include "angelscript.h"

#include "utils/file.hpp"
#include "math/math.hpp"
#include "utils/string.hpp"

#include "fmt/args.h"
#include "fmt/core.h"

#define AS_CALL(f)                                                       \
	do {                                                                 \
		int r = f;                                                       \
		if (r < 0) {                                                     \
			Debug::Error("Script Error : at {}:{}", __FILE__, __LINE__); \
		}                                                                \
	} while (false)
#define AS_CALL_SET(f, v)                                                \
	do {                                                                 \
		int r_aaaaaaaaaaaaaaaaaaaaaaa = f;                               \
		if (r < 0) {                                                     \
			Debug::Error("Script Error : at {}:{}", __FILE__, __LINE__); \
		}                                                                \
                                                                         \
		v = r_aaaaaaaaaaaaaaaaaaaaaaa;                                   \
	} while (false)

namespace sowa {
script_server::script_server(sowa::EngineContext &ctx)
	: _Context(ctx) {
}
script_server::~script_server() {}

script_server &script_server::Of(sowa::EngineContext *context) {
	return *context->GetSingleton<script_server>(sowa::Server::PROJECT);
}

void MessageCallback(const asSMessageInfo *msg, void *param) {
	const char *type = "ERR ";
	if (msg->type == asMSGTYPE_WARNING)
		type = "WARN";
	else if (msg->type == asMSGTYPE_INFORMATION)
		type = "INFO";

	if (msg->type == asMSGTYPE_INFORMATION)
		Debug::Info("{} ({}, {}) : {}", msg->section, msg->row, msg->col, msg->message);
	else if (msg->type == asMSGTYPE_WARNING)
		Debug::Warn("{} ({}, {}) : {}", msg->section, msg->row, msg->col, msg->message);
	else if (msg->type == asMSGTYPE_ERROR)
		Debug::Error("{} ({}, {}) : {}", msg->section, msg->row, msg->col, msg->message);
}

void Print(const std::string &s) {
	Debug::Log(s);
}

std::string Format_Generic(asIScriptGeneric *gen) {
	std::string msg = "FORMAT ERROR!";

	if (gen->GetArgCount() == 0) {
		return msg;
	}

	int fmtTypeId = gen->GetArgTypeId(0);
	asITypeInfo *fmtType = gen->GetEngine()->GetTypeInfoById(fmtTypeId);
	std::string fmtTypename = fmtType->GetName();

	if (fmtTypename != "string") {
		return msg;
	}

	std::string format = *(std::string *)gen->GetArgAddress(0);
	fmt::dynamic_format_arg_store<fmt::format_context> store;
	for (size_t i = 1; i < gen->GetArgCount(); i++) {
		void *ref = gen->GetArgAddress(i);
		int typeId = gen->GetArgTypeId(i);

		if (typeId == asTYPEID_VOID) {
			store.push_back("null");
		} else if (typeId == asTYPEID_INT32) {
			store.push_back(*reinterpret_cast<int32_t *>(ref));
		} else if (typeId == asTYPEID_FLOAT) {
			store.push_back(*reinterpret_cast<float *>(ref));
		} else {
			asITypeInfo *t = gen->GetEngine()->GetTypeInfoById(typeId);
			std::string tname = t->GetName();

			if (tname == "string") {
				store.push_back(*reinterpret_cast<std::string *>(ref));
				continue;
			}
			asIScriptFunction *reprFunc = t->GetMethodByDecl("string _repr()");

			asIScriptContext *ctx = gen->GetEngine()->CreateContext();
			ctx->Prepare(reprFunc);
			ctx->SetObject(ref);
			int r;
			AS_CALL_SET(ctx->Execute(), r);
			if (r == asEXECUTION_FINISHED) {
				void *repr = ctx->GetReturnObject();
				store.push_back(*reinterpret_cast<std::string *>(repr));
				continue;

			} else if (r == asEXECUTION_EXCEPTION) {
				auto func = ctx->GetExceptionFunction();
				Debug::Error("Angelscript error: \n\tdesc: {}\n\tfunc: {}\n\tmodl: {}\n\tsect: {}\n\n\tline: {}", ctx->GetExceptionString(), func->GetDeclaration(), func->GetModuleName(), func->GetScriptSectionName(), ctx->GetExceptionLineNumber());
				return msg;
			}
			store.push_back("Type:" + tname);
		}
	}

	try {
		msg = fmt::vformat(format, store);
	} catch (const std::exception &e) {
		Debug::Error("String Formatting error: {}", e.what());
		return msg;
	}

	return msg;
}

static void Print_Log(asIScriptGeneric *gen) {
	std::string msg = Format_Generic(gen);
	Debug::Log(msg);
}
static void Print_Info(asIScriptGeneric *gen) {
	std::string msg = Format_Generic(gen);
	Debug::Info(msg);
}
static void Print_Warn(asIScriptGeneric *gen) {
	std::string msg = Format_Generic(gen);
	Debug::Warn(msg);
}
static void Print_Error(asIScriptGeneric *gen) {
	std::string msg = Format_Generic(gen);
	Debug::Error(msg);
}
static void Format_Generic_AS(asIScriptGeneric *gen) {
	void *obj = gen->GetEngine()->CreateScriptObject(gen->GetEngine()->GetTypeInfoByName("string"));
	*reinterpret_cast<std::string *>(obj) = Format_Generic(gen);
	gen->SetReturnObject(obj);
}

template <typename T>
void Generic_Constructor(void *memory) {
	new (memory) T;
}

template <typename T>
void Generic_Destructor(void *memory) {
	((T*)memory)->~T();
}

void script_server::init() {
	asIScriptEngine *engine = asCreateScriptEngine();
	AS_CALL(engine->SetMessageCallback(asFUNCTION(MessageCallback), 0, asCALL_CDECL));

	RegisterStdString(engine);

	AS_CALL(engine->SetDefaultNamespace("Debug"));

	std::string genericDecl = " {}(string &in";
	for (int i = 0; i < 16; i++)
		genericDecl += fmt::format(", ?&in var{} = \"\"", i);
	genericDecl += ")";
	std::string printDecl = "void" + genericDecl;

	AS_CALL(engine->RegisterGlobalFunction(sowa::Format(printDecl, "Log").c_str(), asFUNCTION(Print_Log), asCALL_GENERIC));
	AS_CALL(engine->RegisterGlobalFunction(sowa::Format(printDecl, "Info").c_str(), asFUNCTION(Print_Info), asCALL_GENERIC));
	AS_CALL(engine->RegisterGlobalFunction(sowa::Format(printDecl, "Error").c_str(), asFUNCTION(Print_Error), asCALL_GENERIC));
	AS_CALL(engine->RegisterGlobalFunction(sowa::Format(printDecl, "Warn").c_str(), asFUNCTION(Print_Warn), asCALL_GENERIC));
	AS_CALL(engine->SetDefaultNamespace(""));

	std::string fmtDecl = "string" + genericDecl;
	AS_CALL(engine->RegisterGlobalFunction(sowa::Format(fmtDecl, "Format").c_str(), asFUNCTION(Format_Generic_AS), asCALL_GENERIC));

	AS_CALL(engine->RegisterObjectType("vec2", sizeof(sowa::vec2), asOBJ_VALUE));
	AS_CALL(engine->RegisterObjectBehaviour("vec2", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(Generic_Constructor<sowa::vec2>), asCALL_CDECL_OBJLAST));
	AS_CALL(engine->RegisterObjectBehaviour("vec2", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(Generic_Destructor<sowa::vec2>), asCALL_CDECL_OBJLAST));
	AS_CALL(engine->RegisterObjectProperty("vec2", "float x", asOFFSET(sowa::vec2, x)));
	AS_CALL(engine->RegisterObjectProperty("vec2", "float y", asOFFSET(sowa::vec2, y)));

	int r;
	CScriptBuilder builder;
	r = builder.StartNewModule(engine, "Main");
	if (r < 0) {
		throw std::runtime_error("Failed to initalize script module");
	}

	auto f = sowa::File::GetFileContent("res://src/main.as");
	r = builder.AddSectionFromMemory("file.as", reinterpret_cast<const char *>(f.data()), f.size());
	if (r < 0) {
		throw std::runtime_error("Failed to add script section");
	}

	r = builder.BuildModule();
	if (r < 0) {
		throw std::runtime_error("Failed to build module");
	}

	asIScriptModule *mod = engine->GetModule("Main");
	asIScriptFunction *func = mod->GetFunctionByDecl("void main()");
	if (func == 0) {
		throw std::runtime_error("Failed to find main function");
	}

	asIScriptContext *ctx = engine->CreateContext();
	ctx->Prepare(func);
	r = ctx->Execute();
	if (r != asEXECUTION_FINISHED) {
		if (r == asEXECUTION_EXCEPTION) {
			auto func = ctx->GetExceptionFunction();
			Debug::Error("Angelscript error: \n\tdesc: {}\n\tfunc: {}\n\tmodl: {}\n\tsect: {}\n\n\tline: {}", ctx->GetExceptionString(), func->GetDeclaration(), func->GetModuleName(), func->GetScriptSectionName(), ctx->GetExceptionLineNumber());
			throw std::runtime_error("Script exception");
		}
	}

	ctx->Release();
	engine->ShutDownAndRelease();
}

} // namespace sowa
