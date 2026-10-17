#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "build/build_params.hpp"
#include "build/file_rules.hpp"
#include "core/project_settings.hpp"

namespace sowa::build {

inline constexpr std::string_view kBuildScriptFile = "build.lua";

enum class BuildPhase {
	Configure,
	Stage,
	Staged,
	Done,
	Failed,
};

// Everything one target's build knows, shared between BuildScript (which
// fills the rules and project overrides from Lua) and the runner.
struct BuildTarget {
	std::string name;
	std::string platform;
	std::vector<std::pair<std::string, ParamValue>> params;
	// Starts as project.sowa; ctx.project writes land here after configure.
	ProjectSettings project;
	std::filesystem::path output;
	// ctx:mount("res") and ctx:output_dir() rules, in call order.
	std::vector<FileRule> resRules;
	std::vector<FileRule> outputRules;
	ResolveContext resolve;
	BuildPhase phase = BuildPhase::Configure;
	// Set by ctx:abort.
	bool aborted = false;
	std::string abortMessage;
};

// A build.lua running in its own sandboxed Lua state. One instance per build
// run: Load once, then Configure/Emit/RunWriteFunction per target.
class BuildScript {
  public:
	BuildScript();
	~BuildScript();

	BuildScript(const BuildScript &) = delete;
	BuildScript &operator=(const BuildScript &) = delete;

	// Runs build.lua (or the built-in default when the project has none) and
	// validates its declarations.
	bool Load(const std::filesystem::path &projectRoot, std::string &error);

	bool HasBuildFile() const;
	const std::vector<ParamDecl> &Params() const;
	const std::vector<std::string> &Platforms() const;
	// Unix seconds captured when Load ran (build.timestamp()).
	std::int64_t Timestamp() const;

	// Creates ctx for target, runs the configure handlers and stores the
	// resulting rules and ctx.project in target.
	bool Configure(BuildTarget &target, std::string &error);

	// Calls write_fn number index for target, appending what it writes.
	bool RunWriteFunction(BuildTarget &target, int index, std::string &bytes, std::string &error);

	// Runs "staged" or "done" handlers.
	bool Emit(std::string_view event, BuildTarget &target, std::string &error);

	// Runs "failed" handlers; their own errors are logged, not returned.
	void EmitFailed(BuildTarget &target, const std::string &message);

  private:
	struct Impl;
	std::unique_ptr<Impl> _impl;
};

} // namespace sowa::build
