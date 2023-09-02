#ifndef SW_SCRIPT_SERVER_HXX
#define SW_SCRIPT_SERVER_HXX
#pragma once

#include <angelscript.h>
#include <string>

#include "core/error/error.hxx"

class ScriptServer {
  public:
	static ScriptServer &GetInstance();

	void BeginBuild();
	Error LoadScriptFile(std::string path);
	void EndBuild();

  private:
	void register_script_behaviour();

	ScriptServer();
	~ScriptServer();
};

#endif // SW_SCRIPT_SERVER_HXX