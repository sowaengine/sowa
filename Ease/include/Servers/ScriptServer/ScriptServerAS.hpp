#ifndef _E_SCRIPTSERVERAS_HPP__
#define _E_SCRIPTSERVERAS_HPP__
#pragma once

#include "Servers/ScriptServer/ScriptServer.hpp"

#include "angelscript/angelscript.h"

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

  private:
	friend class Application;
	ScriptServerAS();
	~ScriptServerAS();

	static ScriptServerAS *CreateServer() {
		return new ScriptServerAS;
	}

  private:
	asIScriptEngine *_pEngine{nullptr};
	asIScriptContext *_pContext{nullptr};
};
} // namespace Ease

#endif // _E_SCRIPTSERVER_HPP__