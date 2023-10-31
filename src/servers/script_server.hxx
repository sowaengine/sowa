#ifndef SW_SCRIPT_SERVER_HXX
#define SW_SCRIPT_SERVER_HXX
#pragma once

#include <angelscript.h>
#include <string>

#include "core/error/error.hxx"

struct ScriptFunctionCaller {
  public:
	ScriptFunctionCaller &arg(); /// Blank arg to increment arg counter

	ScriptFunctionCaller &arg_u32(uint32_t value);
	ScriptFunctionCaller &arg_u64(uint64_t value);

	void call();
	~ScriptFunctionCaller();

  private:
	friend class ScriptServer;
	ScriptFunctionCaller() = delete;

	ScriptFunctionCaller(const std::string &moduleName, const std::string &decl);

	int m_arg_counter;
	asIScriptContext *ctx;
};

class ScriptServer {
  public:
	static ScriptServer &get();

	void BeginBuild();
	ErrorCode LoadScriptFile(std::string path);
	void EndBuild();

	inline ScriptFunctionCaller begin_func(const std::string &decl) {
		return ScriptFunctionCaller("Sowa", decl);
	}

  private:
	void register_script_behaviour();

	ScriptServer();
	~ScriptServer();
};

#endif // SW_SCRIPT_SERVER_HXX