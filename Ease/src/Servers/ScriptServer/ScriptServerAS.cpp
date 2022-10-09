#include "Servers/ScriptServer/ScriptServerAS.hpp"
#include "Debug.hpp"
#include "Servers/ScriptServer/ASContext.hpp"

#include "Core/Application.hpp"
#include "Core/ProjectSettings.hpp"
#include "Servers/GuiServer/GuiServer.hpp"

#include "add_on/scriptarray/scriptarray.h"
#include "add_on/scriptbuilder/scriptbuilder.h"
#include "add_on/scriptstdstring/scriptstdstring.h"
#include "angelscript.h"

static void ASMessageCallback(const asSMessageInfo *msg, void *param) {
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

#define AS_CALL(f, ...)                        \
	do {                                       \
		int r = f(__VA_ARGS__);                \
		if (r < 0) {                           \
			Debug::Error("AS_CALL: {}()", #f); \
		}                                      \
	} while (false)

namespace Ease {
ScriptServerAS::ScriptServerAS(EngineContext &ctx) : _Context(ctx) {
	_pEngine = asCreateScriptEngine();
	AS_CALL(_pEngine->SetMessageCallback, asFUNCTION(ASMessageCallback), 0, asCALL_CDECL);
	RegisterScriptArray(_pEngine, false);
	RegisterStdString(_pEngine);
	RegisterStdStringUtils(_pEngine);
	ASContext::RegisterReprFunc("string", [](void *obj) { return *(std::string *)obj; });

	// Register generic functions, types

	//* vv<Debug>vv */
	SetNamespace("Debug");

	std::string printdecl = "void {}(string &in";
	for (int i = 0; i < 16; i++)
		printdecl += fmt::format(", ?&in var{} = \"\"", i);
	printdecl += ")";
	// void print(string &in, ?&in var0 = "" ...)
	AS_CALL(_pEngine->RegisterGlobalFunction, fmt::format(printdecl, "Log").c_str(), asFUNCTION(ASContext::Log_Generic), asCALL_GENERIC);
	AS_CALL(_pEngine->RegisterGlobalFunction, fmt::format(printdecl, "Info").c_str(), asFUNCTION(ASContext::Info_Generic), asCALL_GENERIC);
	AS_CALL(_pEngine->RegisterGlobalFunction, fmt::format(printdecl, "Warn").c_str(), asFUNCTION(ASContext::Warn_Generic), asCALL_GENERIC);
	AS_CALL(_pEngine->RegisterGlobalFunction, fmt::format(printdecl, "Error").c_str(), asFUNCTION(ASContext::Error_Generic), asCALL_GENERIC);

	//* --<Debug>-- */
	SetNamespace("");

	//* vv<Math>vv */
	ASContext::RegisterReprFunc("Vector2", ASContext::ASVector2::_Repr);
	RegisterRefType("Vector2")
		.RefFunc("void f()", asMETHOD(ASContext::ASVector2, _AddRef))
		.ReleaseFunc("void f()", asMETHOD(ASContext::ASVector2, _Release))
		.Factory("Vector2@ f()", asFUNCTIONPR(ASContext::ASRefCounted::_Create, (), ASContext::ASVector2 *))
		.Factory("Vector2@ f(float)", asFUNCTIONPR(ASContext::ASRefCounted::_Create, (float), ASContext::ASVector2 *))
		.Factory("Vector2@ f(float, float)", asFUNCTIONPR(ASContext::ASRefCounted::_Create, (float, float), ASContext::ASVector2 *))
		.Factory("Vector2@ f(Vector2@)", asFUNCTIONPR(ASContext::ASRefCounted::_Create, (const ASContext::ASVector2 &), ASContext::ASVector2 *))
		.Method("Vector2 &opAssign(const Vector2&in)", asMETHODPR(ASContext::ASVector2, operator=, (const ASContext::ASVector2 &), ASContext::ASVector2 &))
		.Property("float x", asOFFSET(ASContext::ASVector2, vec.x))
		.Property("float y", asOFFSET(ASContext::ASVector2, vec.y))
		.Method("float Length()", asMETHOD(ASContext::ASVector2, Length))
		.Method("Vector2@ Clamp(float = 1.0)", asMETHOD(ASContext::ASVector2, Clamp))
		.Method("float Angle()", asMETHOD(ASContext::ASVector2, Angle));

	//* --<Math>-- */

	//* vv<Gui>vv */
	SetNamespace("Gui");
	GuiServer *guiServer = _Context.GetSingleton<GuiServer>(Server::GUISERVER);
	static ASContext::GuiCaller guiCaller(guiServer);

	AS_CALL(_pEngine->RegisterEnum, "StyleVar");
	AS_CALL(_pEngine->RegisterEnum, "WindowFlags");

	AS_CALL(_pEngine->RegisterGlobalFunction, "void BeginWindow(string, uint = 0)", asMETHOD(GuiServer, BeginWindow), asCALL_THISCALL_ASGLOBAL, guiServer);
	AS_CALL(_pEngine->RegisterGlobalFunction, "void EndWindow()", asMETHOD(GuiServer, EndWindow), asCALL_THISCALL_ASGLOBAL, guiServer);
	AS_CALL(_pEngine->RegisterGlobalFunction, "void Text(string)", asMETHOD(GuiServer, Text), asCALL_THISCALL_ASGLOBAL, guiServer);
	AS_CALL(_pEngine->RegisterGlobalFunction, "bool Button(string, int = 0, int = 0)", asMETHOD(GuiServer, Button), asCALL_THISCALL_ASGLOBAL, guiServer);
	AS_CALL(_pEngine->RegisterGlobalFunction, "bool DragFloat(string, float &out)", asMETHOD(GuiServer, DragFloat), asCALL_THISCALL_ASGLOBAL, guiServer);
	AS_CALL(_pEngine->RegisterGlobalFunction, "void SetNextWindowPos(int, int)", asMETHOD(GuiServer, SetNextWindowPos), asCALL_THISCALL_ASGLOBAL, guiServer);
	AS_CALL(_pEngine->RegisterGlobalFunction, "void SetNextWindowSize(int, int)", asMETHOD(GuiServer, SetNextWindowSize), asCALL_THISCALL_ASGLOBAL, guiServer);
	AS_CALL(_pEngine->RegisterGlobalFunction, "void PushStyleVar(StyleVar, float)", asMETHODPR(GuiServer, PushStyleVar, (StyleVar, float), void), asCALL_THISCALL_ASGLOBAL, guiServer);
	AS_CALL(_pEngine->RegisterGlobalFunction, "void PushStyleVar(StyleVar, Vector2@)", asMETHODPR(ASContext::GuiCaller, PushStyleVar, (StyleVar, ASContext::ASVector2 *), void), asCALL_THISCALL_ASGLOBAL, &guiCaller);
	AS_CALL(_pEngine->RegisterGlobalFunction, "void PopStyleVar(int = 1)", asMETHOD(GuiServer, PopStyleVar), asCALL_THISCALL_ASGLOBAL, guiServer);

	//
	AS_CALL(_pEngine->RegisterEnumValue, "WindowFlags", "None", WindowFlags_None);
	AS_CALL(_pEngine->RegisterEnumValue, "WindowFlags", "NoResize", WindowFlags_NoResize);
	AS_CALL(_pEngine->RegisterEnumValue, "WindowFlags", "NoMove", WindowFlags_NoMove);
	AS_CALL(_pEngine->RegisterEnumValue, "WindowFlags", "NoBringToFrontOnFocus", WindowFlags_NoBringToFrontOnFocus);

	AS_CALL(_pEngine->RegisterEnumValue, "StyleVar", "None", (int)StyleVar::None);
	AS_CALL(_pEngine->RegisterEnumValue, "StyleVar", "WindowPadding", (int)StyleVar::WindowPadding);
	AS_CALL(_pEngine->RegisterEnumValue, "StyleVar", "WindowRounding", (int)StyleVar::WindowRounding);
	//* --<Gui>-- */
	SetNamespace("");

	//* vv<Window>vv */
	SetNamespace("Window");
	auto *app = _Context.GetSingleton<Application>(Server::APPLICATION);
	auto &window = app->GetWindow();
	static ASContext::WindowCaller windowCaller(&window);

	AS_CALL(_pEngine->RegisterGlobalFunction, "Vector2@ GetWindowSize()", asMETHOD(ASContext::WindowCaller, GetWindowSize), asCALL_THISCALL_ASGLOBAL, &windowCaller);

	//* --<Window>-- */
	SetNamespace("");
}
ScriptServerAS::~ScriptServerAS() {
	if (_pContext != nullptr)
		AS_CALL(_pContext->Release);
	AS_CALL(_pEngine->ShutDownAndRelease);
}

void ScriptServerAS::CreateModule(const char *moduleName) {
	if (_pEngine->GetModule(moduleName, asGM_ONLY_IF_EXISTS) != nullptr) {
		Debug::Warn("AS Module {} already exists. Cannot create", moduleName);
	}

	// _pCurrentModule = _pEngine->GetModule(moduleName, asGM_ONLY_IF_EXISTS);
	CScriptBuilder builder;
	AS_CALL(builder.StartNewModule, _pEngine, moduleName);
	AS_CALL(builder.BuildModule);
}

void ScriptServerAS::LoadScript(const char *moduleName, const char *fileName, std::vector<unsigned char> &file) {
	std::string code{reinterpret_cast<char *>(file.data()), file.size()};

	asIScriptModule *mod = _pEngine->GetModule(moduleName, asGM_ONLY_IF_EXISTS);
	if (mod == nullptr) {
		Debug::Error("AS Module {} doesn't exists, cannot load script {}", moduleName, fileName);
		return;
	}

	CScriptBuilder builder;
	AS_CALL(mod->AddScriptSection, fileName, code.c_str());
	AS_CALL(mod->Build);
}

void ScriptServerAS::BeginContext() {
	_pContext = _pEngine->CreateContext();
}

void ScriptServerAS::CallFunc(const char *moduleName, const char *funcDecl) {
	if (_pEngine->GetModule(moduleName, asGM_ONLY_IF_EXISTS) == nullptr) {
		Debug::Warn("AS Module {} doesn't exists, creating new one.", moduleName);
	}

	asIScriptModule *mod = _pEngine->GetModule(moduleName, asGM_CREATE_IF_NOT_EXISTS);
	asIScriptFunction *func = mod->GetFunctionByDecl(funcDecl);
	if (func == nullptr) {
		Debug::Error("Can't call {}, function doesn't exists.", funcDecl);
		return;
	}

	AS_CALL(_pContext->Prepare, func);
	int r = _pContext->Execute();
	if (r != asEXECUTION_FINISHED) {
		if (r == asEXECUTION_EXCEPTION) {
			Debug::Error("Exception on Module {} at line {} call {}: {}", moduleName, _pContext->GetLineNumber(), funcDecl, _pContext->GetExceptionString());
		}
	}
}

void ScriptServerAS::SetNamespace(const char *name) {
	_pEngine->SetDefaultNamespace(name);
}

// unnecessary duplication, add internal method that calls given asIScriptFunction*
void ScriptServerAS::InitModules() {
	BeginContext();
	for (const std::string &path : _Context.GetSingleton<Ease::ProjectSettings>(Ease::Server::PROJECTSETTINGS)->_modules.as) {
		std::filesystem::path fullpath = Ease::File::Path(path);
		std::string moduleName = fullpath.filename().replace_extension("").string();

		std::vector<unsigned char> code = Ease::File::GetFileContent(path.c_str());
		CreateModule(moduleName.c_str());
		LoadScript(moduleName.c_str(), fullpath.filename().string().c_str(), code);

		Debug::Info("Load module '{}' at '{}'", moduleName, fullpath.string());
	}
	for (size_t i = 0; i < _pEngine->GetModuleCount(); i++) {
		asIScriptModule *_module = _pEngine->GetModuleByIndex(i);
		if (asIScriptFunction *func = _module->GetFunctionByDecl("void init()"); func != nullptr) {
			AS_CALL(_pContext->Prepare, func);
			int r = _pContext->Execute();
			if (r != asEXECUTION_FINISHED) {
				if (r == asEXECUTION_EXCEPTION) {
					Debug::Error("Exception on Module {} at line {} call {}: {}", _module->GetName(), _pContext->GetLineNumber(), "void init()", _pContext->GetExceptionString());
				}
			}
		}
	}
}

void ScriptServerAS::UpdateModules() {
	for (size_t i = 0; i < _pEngine->GetModuleCount(); i++) {
		asIScriptModule *_module = _pEngine->GetModuleByIndex(i);
		if (asIScriptFunction *func = _module->GetFunctionByDecl("void update()"); func != nullptr) {
			AS_CALL(_pContext->Prepare, func);
			int r = _pContext->Execute();
			if (r != asEXECUTION_FINISHED) {
				if (r == asEXECUTION_EXCEPTION) {
					Debug::Error("Exception on Module {} at line {} call {}: {}", _module->GetName(), _pContext->GetLineNumber(), "void update()", _pContext->GetExceptionString());
				}
			}
		}
	}
}

void ScriptServerAS::GuiUpdateModules() {
	for (size_t i = 0; i < _pEngine->GetModuleCount(); i++) {
		asIScriptModule *_module = _pEngine->GetModuleByIndex(i);
		if (asIScriptFunction *func = _module->GetFunctionByDecl("void gui_update()"); func != nullptr) {
			AS_CALL(_pContext->Prepare, func);
			int r = _pContext->Execute();
			if (r != asEXECUTION_FINISHED) {
				if (r == asEXECUTION_EXCEPTION) {
					Debug::Error("Exception on Module {} at line {} call {}: {}", _module->GetName(), _pContext->GetLineNumber(), "void gui_update()", _pContext->GetExceptionString());
				}
			}
		}
	}
}

ScriptServerAS::TypeRegistrar::TypeRegistrar(asIScriptEngine *engine, const std::string &_typename) {
	_pEngine = engine;
	_TypeName = _typename;
}
ScriptServerAS::TypeRegistrar::~TypeRegistrar() {
}

ScriptServerAS::TypeRegistrar ScriptServerAS::RegisterRefType(const std::string &_typename) {
	TypeRegistrar registrar(_pEngine, _typename);
	AS_CALL(_pEngine->RegisterObjectType, _typename.c_str(), 0, asOBJ_REF);
	return registrar;
}
ScriptServerAS::TypeRegistrar ScriptServerAS::RegisterValueType(const std::string &_typename) {
	TypeRegistrar registrar(_pEngine, _typename);
	AS_CALL(_pEngine->RegisterObjectType, _typename.c_str(), 0, asOBJ_REF | asOBJ_NOCOUNT);
	return registrar;
}

ScriptServerAS::TypeRegistrar &ScriptServerAS::TypeRegistrar::Factory(const char *decl, const asSFuncPtr &funcPointer) {
	AS_CALL(_pEngine->RegisterObjectBehaviour, _TypeName.c_str(), asBEHAVE_FACTORY, decl, funcPointer, asCALL_CDECL);
	return *this;
}
ScriptServerAS::TypeRegistrar &ScriptServerAS::TypeRegistrar::RefFunc(const char *decl, const asSFuncPtr &funcPointer) {
	AS_CALL(_pEngine->RegisterObjectBehaviour, _TypeName.c_str(), asBEHAVE_ADDREF, decl, funcPointer, asCALL_THISCALL);
	return *this;
}
ScriptServerAS::TypeRegistrar &ScriptServerAS::TypeRegistrar::ReleaseFunc(const char *decl, const asSFuncPtr &funcPointer) {
	AS_CALL(_pEngine->RegisterObjectBehaviour, _TypeName.c_str(), asBEHAVE_RELEASE, decl, funcPointer, asCALL_THISCALL);
	return *this;
}
ScriptServerAS::TypeRegistrar &ScriptServerAS::TypeRegistrar::Method(const char *decl, const asSFuncPtr &funcPointer) {
	AS_CALL(_pEngine->RegisterObjectMethod, _TypeName.c_str(), decl, funcPointer, asCALL_THISCALL);
	return *this;
}
ScriptServerAS::TypeRegistrar &ScriptServerAS::TypeRegistrar::Property(const char *decl, int byteOffset) {
	AS_CALL(_pEngine->RegisterObjectProperty, _TypeName.c_str(), decl, byteOffset);
	return *this;
}

} // namespace Ease
