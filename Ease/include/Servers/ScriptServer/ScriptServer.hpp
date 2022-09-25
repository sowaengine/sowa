#ifndef _E_SCRIPTSERVER_HPP__
#define _E_SCRIPTSERVER_HPP__
#pragma once

#include <string>
#include <vector>

namespace Ease {
class ScriptServer {
  public:
	virtual void CreateModule(const char *moduleName) = 0;
	virtual void LoadScript(const char *moduleName, const char *fileName, std::vector<unsigned char> &file) = 0;

	virtual void BeginContext() = 0;
	virtual void CallFunc(const char *moduleName, const char *funcDecl) = 0;

	virtual void SetNamespace(const char *name) = 0;

	ScriptServer(const ScriptServer &) = delete;
	ScriptServer(const ScriptServer &&) = delete;
	ScriptServer &operator=(const ScriptServer &) = delete;
	ScriptServer &operator=(const ScriptServer &&) = delete;

  protected:
	ScriptServer();
	virtual ~ScriptServer();
};
} // namespace Ease

#endif // _E_SCRIPTSERVER_HPP__