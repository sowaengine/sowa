#ifndef SW_SCRIPT_SERVER_HXX
#define SW_SCRIPT_SERVER_HXX
#pragma once

#include <angelscript.h>
#include <string>

#include "core/error/error.hxx"

struct ScriptFunctionCaller {
  public:
	ScriptFunctionCaller &arg(); /// Blank arg to increment arg counter
	ScriptFunctionCaller &arg_this(void *o);

	ScriptFunctionCaller &arg_object(void *o);
	ScriptFunctionCaller &arg_u32(uint32_t value);
	ScriptFunctionCaller &arg_u64(uint64_t value);

	void call();

	ScriptFunctionCaller();
	~ScriptFunctionCaller();

  private:
	friend class ScriptServer;

	ScriptFunctionCaller(const std::string &moduleName, const std::string &decl);
	ScriptFunctionCaller(const std::string &moduleName, const std::string className, const std::string &decl);

#ifndef SW_WEB
	int m_arg_counter;
	asIScriptContext *ctx;
#endif
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

	inline ScriptFunctionCaller begin_method(const std::string &className, const std::string &decl) {
		return ScriptFunctionCaller("Sowa", className, decl);
	}

  private:
	void register_script_behaviour();

	ScriptServer();
	~ScriptServer();
};

#endif // SW_SCRIPT_SERVER_HXX