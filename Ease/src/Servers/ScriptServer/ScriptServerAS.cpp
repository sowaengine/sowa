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
	AS_CALL(_pEngine->RegisterEnum, "TreeFlags");
	AS_CALL(_pEngine->RegisterEnum, "GuiMouseButton");

	struct FuncDecl {
		std::string decl;
		asSFuncPtr func;
		void *obj{nullptr};
		FuncDecl(const std::string &_decl, asSFuncPtr _func, void *_obj) : decl(_decl), func(_func), obj(_obj) {}
	};
	std::vector<FuncDecl> gui_functions = {
		{"bool BeginWindow(string, uint = 0)", asMETHOD(GuiServer, BeginWindow), guiServer},
		{"void EndWindow()", asMETHOD(GuiServer, EndWindow), guiServer},

		{"void Text(string)", asMETHOD(GuiServer, Text), guiServer},
		{"bool Button(string, int = 0, int = 0)", asMETHOD(GuiServer, Button), guiServer},
		{"bool DragFloat(string, float &out)", asMETHOD(GuiServer, DragFloat), guiServer},
		{"bool Checkbox(string, bool &out)", asMETHOD(GuiServer, Checkbox), guiServer},
		{"void Separator()", asMETHOD(GuiServer, Separator), guiServer},

		{"void SetNextWindowPos(int, int)", asMETHOD(GuiServer, SetNextWindowPos), guiServer},
		{"void SetNextWindowSize(int, int)", asMETHOD(GuiServer, SetNextWindowSize), guiServer},

		{"void PushStyleVar(StyleVar, float)", asMETHODPR(GuiServer, PushStyleVar, (StyleVar, float), void), guiServer},
		{"void PushStyleVar(StyleVar, Vector2@)", asMETHODPR(ASContext::GuiCaller, PushStyleVar, (StyleVar, ASContext::ASVector2 *), void), &guiCaller},
		{"void PopStyleVar(int = 1)", asMETHOD(GuiServer, PopStyleVar), guiServer},

		{"void SetupDockspace()", asMETHOD(GuiServer, SetupDockspace), guiServer},

		{"bool BeginMainMenuBar()", asMETHOD(GuiServer, BeginMainMenuBar), guiServer},
		{"void EndMainMenuBar()", asMETHOD(GuiServer, EndMainMenuBar), guiServer},
		{"bool BeginMenu(string)", asMETHOD(GuiServer, BeginMenu), guiServer},
		{"void EndMenu()", asMETHOD(GuiServer, EndMenu), guiServer},
		{"bool MenuItem(string, string = \"\")", asMETHOD(GuiServer, MenuItem), guiServer},

		{"bool BeginFooter(string)", asMETHOD(GuiServer, BeginFooter), guiServer},
		{"void EndFooter()", asMETHOD(GuiServer, EndFooter), guiServer},

		{"void RegisterPanel(string, string, bool = true)", asMETHOD(GuiServer, RegisterPanel), guiServer},
		{"void DrawViewbar()", asMETHOD(GuiServer, DrawViewbar), guiServer},

		{"bool BeginCheckerList(string)", asMETHOD(GuiServer, BeginCheckerList), guiServer},
		{"void CheckerListNextItem()", asMETHOD(GuiServer, CheckerListNextItem), guiServer},
		{"void EndCheckerList()", asMETHOD(GuiServer, EndCheckerList), guiServer},

		{"bool BeginTree(string, uint = 0)", asMETHOD(GuiServer, BeginTree), guiServer},
		{"void EndTree()", asMETHOD(GuiServer, EndTree), guiServer},

		{"void PushID(string)", asMETHOD(GuiServer, PushID), guiServer},
		{"void PopID()", asMETHOD(GuiServer, PopID), guiServer},

		{"void ShowDemoWindow()", asMETHOD(GuiServer, ShowDemoWindow), guiServer},

		{"void DrawFrame()", asMETHOD(GuiServer, DrawFrame), guiServer},
		{"void DrawFilesystem()", asMETHOD(GuiServer, DrawFilesystem), guiServer},
		{"void DrawPlayButton()", asMETHOD(GuiServer, DrawPlayButton), guiServer},
		{"void DrawScene()", asMETHOD(GuiServer, DrawScene), guiServer},

		{"bool IsWindowHovered()", asMETHOD(GuiServer, IsWindowHovered), guiServer},
		{"bool IsMouseClicked(GuiMouseButton)", asMETHOD(GuiServer, IsMouseClicked), guiServer},
		{"bool IsMouseDoubleClicked(GuiMouseButton)", asMETHOD(GuiServer, IsMouseDoubleClicked), guiServer},

		{"void OpenContextMenu(string)", asMETHOD(GuiServer, OpenContextMenu), guiServer},
		{"bool BeginContextMenu(string)", asMETHOD(GuiServer, BeginContextMenu), guiServer},
		{"void EndContextMenu()", asMETHOD(GuiServer, EndContextMenu), guiServer},
	};

	for (auto &[decl, func, obj] : gui_functions) {
		AS_CALL(_pEngine->RegisterGlobalFunction, decl.c_str(), func, asCALL_THISCALL_ASGLOBAL, obj);
	}

	struct EnumDecl {
		std::string type;
		std::string name;
		int value;
		EnumDecl(const std::string &_type, const std::string &_name, int _value) : type(_type), name(_name), value(_value) {}
	};
	std::vector<EnumDecl> enums = {
		{"WindowFlags", "WindowFlags_None", WindowFlags_None},
		{"WindowFlags", "WindowFlags_NoResize", WindowFlags_NoResize},
		{"WindowFlags", "WindowFlags_NoMove", WindowFlags_NoMove},
		{"WindowFlags", "WindowFlags_NoBringToFrontOnFocus", WindowFlags_NoBringToFrontOnFocus},
		{"WindowFlags", "WindowFlags_NoNavFocus", WindowFlags_NoNavFocus},
		{"WindowFlags", "WindowFlags_NoDocking", WindowFlags_NoDocking},
		{"WindowFlags", "WindowFlags_NoTitleBar", WindowFlags_NoTitleBar},
		{"WindowFlags", "WindowFlags_NoCollapse", WindowFlags_NoCollapse},
		{"WindowFlags", "WindowFlags_MenuBar", WindowFlags_MenuBar},
		{"WindowFlags", "WindowFlags_NoBackground", WindowFlags_NoBackground},

		{"StyleVar", "StyleVar_None", (int)StyleVar::None},
		{"StyleVar", "StyleVar_WindowPadding", (int)StyleVar::WindowPadding},
		{"StyleVar", "StyleVar_WindowRounding", (int)StyleVar::WindowRounding},

		{"TreeFlags", "TreeFlags_None", TreeFlags_None},
		{"TreeFlags", "TreeFlags_Leaf", TreeFlags_Leaf},
		{"TreeFlags", "TreeFlags_SpanMinWidth", TreeFlags_SpanMinWidth},

		{"GuiMouseButton", "GuiMouseButton_None", (int)GuiMouseButton::None},
		{"GuiMouseButton", "GuiMouseButton_Left", (int)GuiMouseButton::Left},
		{"GuiMouseButton", "GuiMouseButton_Right", (int)GuiMouseButton::Right},
		{"GuiMouseButton", "GuiMouseButton_Middle", (int)GuiMouseButton::Middle},
	};

	for (auto &[type, name, value] : enums) {
		AS_CALL(_pEngine->RegisterEnumValue, type.c_str(), name.c_str(), value);
	}
	//* --<Gui>-- */

	//* vv<App>vv */
	SetNamespace("App");
	auto *app = _Context.GetSingleton<Application>(Server::APPLICATION);
	static ASContext::ApplicationCaller appcaller{app};

	RegisterRefType("Entity")
		.RefFunc("void f()", asMETHOD(ASContext::ASEntity, _AddRef))
		.ReleaseFunc("void f()", asMETHOD(ASContext::ASEntity, _Release))
		.Method("Entity &opAssign(const Entity&in)", asMETHODPR(ASContext::ASEntity, operator=, (const ASContext::ASEntity &), ASContext::ASEntity &))
		.Method("void Echo()", asMETHOD(ASContext::ASEntity, Echo));

	RegisterRefType("Scene")
		.RefFunc("void f()", asMETHOD(ASContext::ASScene, _AddRef))
		.ReleaseFunc("void f()", asMETHOD(ASContext::ASScene, _Release))
		.Factory("Scene@ f()", asFUNCTIONPR(ASContext::ASRefCounted::_Create, (), ASContext::ASScene *))
		.Factory("Scene@ f(Scene@)", asFUNCTIONPR(ASContext::ASRefCounted::_Create, (const ASContext::ASScene &), ASContext::ASScene *))
		.Method("Scene &opAssign(const Scene&in)", asMETHODPR(ASContext::ASScene, operator=, (const ASContext::ASScene &), ASContext::ASScene &))
		.Method("bool Valid()", asMETHOD(ASContext::ASScene, Valid))
		.Method("Entity@ Create(string name, uint id = 0)", asMETHOD(ASContext::ASScene, Create));

	std::vector<FuncDecl> app_functions = {
		{"Scene@ GetCurrentScene()", asMETHOD(ASContext::ApplicationCaller, GetCurrentScene), &appcaller},
	};

	for (auto &[decl, func, obj] : app_functions) {
		AS_CALL(_pEngine->RegisterGlobalFunction, decl.c_str(), func, asCALL_THISCALL_ASGLOBAL, obj);
	}

	//* --<App>-- */
	SetNamespace("");

	//* vv<Window>vv */
	SetNamespace("Window");
	auto &window = app->GetWindow();
	static ASContext::WindowCaller windowCaller(&window);

	std::vector<FuncDecl> window_functions = {
		{"Vector2@ GetWindowSize()", asMETHOD(ASContext::WindowCaller, GetWindowSize), &windowCaller},
	};

	for (auto &[decl, func, obj] : window_functions) {
		AS_CALL(_pEngine->RegisterGlobalFunction, decl.c_str(), func, asCALL_THISCALL_ASGLOBAL, obj);
	}

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
		AS_CALL(_module->BindAllImportedFunctions);
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
