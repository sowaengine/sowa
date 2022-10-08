#ifndef _E_SCRIPTSERVERAS_HPP__
#define _E_SCRIPTSERVERAS_HPP__
#pragma once

#include "Core/EngineContext.hpp"
#include "Servers/ScriptServer/ScriptServer.hpp"
#include "angelscript.h"

class asIScriptEngine;
class asIScriptContext;
class asIScriptModule;

namespace Ease {
class ScriptServerAS : public ScriptServer {
  public:
	virtual void CreateModule(const char *moduleName) override;
	virtual void LoadScript(const char *moduleName, const char *fileName, std::vector<unsigned char> &file) override;

	// Call after registering general functions
	virtual void BeginContext() override;
	virtual void CallFunc(const char *moduleName, const char *funcDecl) override;

	virtual void SetNamespace(const char *name) override;

	void InitModules();
	void UpdateModules();
	void GuiUpdateModules();

	class TypeRegistrar {
	  public:
		TypeRegistrar(asIScriptEngine *engine, const std::string &_typename);
		~TypeRegistrar();

		TypeRegistrar &Factory(const char *decl, const asSFuncPtr &funcPointer);
		TypeRegistrar &RefFunc(const char *decl, const asSFuncPtr &funcPointer);
		TypeRegistrar &ReleaseFunc(const char *decl, const asSFuncPtr &funcPointer);
		TypeRegistrar &Method(const char *decl, const asSFuncPtr &funcPointer);
		TypeRegistrar &Property(const char *decl, int byteOffset);

	  private:
		asIScriptEngine *_pEngine{nullptr};
		std::string _TypeName{""};
	};
	TypeRegistrar RegisterRefType(const std::string &_typename);
	TypeRegistrar RegisterValueType(const std::string &_typename);

  private:
	friend class Application;
	ScriptServerAS(EngineContext &ctx);
	~ScriptServerAS();

	static ScriptServerAS *CreateServer(EngineContext &ctx) {
		return new ScriptServerAS(ctx);
	}

	EngineContext &_Context;

  private:
	asIScriptEngine *_pEngine{nullptr};
	asIScriptContext *_pContext{nullptr};
};
} // namespace Ease

#endif // _E_SCRIPTSERVER_HPP__