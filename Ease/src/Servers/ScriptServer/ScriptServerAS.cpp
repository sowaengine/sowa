#include "Servers/ScriptServer/ScriptServerAS.hpp"
#include "Debug.hpp"
#include "angelscript/angelscript.h"
#include "angelscript/scriptarray/scriptarray.h"
#include "angelscript/scriptbuilder/scriptbuilder.h"
#include "angelscript/scriptstdstring/scriptstdstring.h"

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
ScriptServerAS::ScriptServerAS() {
	_pEngine = asCreateScriptEngine();
	AS_CALL(_pEngine->SetMessageCallback, asFUNCTION(ASMessageCallback), 0, asCALL_CDECL);
	RegisterScriptArray(_pEngine, false);
	RegisterStdString(_pEngine);
	RegisterStdStringUtils(_pEngine);
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
			Debug::Error("AS Module {} call {}: {}", moduleName, funcDecl, _pContext->GetExceptionString());
		}
	}
}
} // namespace Ease
