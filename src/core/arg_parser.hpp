#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace sowa {

enum class ArgParseResult {
	Continue,
	ExitSuccess,
	ExitFailure,
};

enum class AppMode {
	Run,
	Edit,
	ProjectManager,
	Build,
};

// sowa build options. Exactly one of list, all or a non-empty
// targets is set after a successful parse.
struct BuildCommand {
	std::vector<std::string> targets;
	bool all = false;
	bool list = false;
	// --param key=value, in command-line order.
	std::vector<std::pair<std::string, std::string>> params;
	// --templates; empty means the default next to the executable.
	std::string templatesDirectory;
};

// Debug adapter options for run and edit.
struct DebugOptions {
	// --debug-port; unset uses the engine default.
	std::optional<std::uint16_t> port;
	// --wait-for-debugger: hold game scripts until VS Code has attached.
	bool waitForDebugger = false;
};

class ArgParser {
  public:
	// Export templates pass false to support only run and default to the packaged ZIP.
	// The archive sits beside the executable. executableDirectory overrides the OS path lookup.
	explicit ArgParser(bool editorAvailable = true, std::filesystem::path executableDirectory = {})
		: _editorAvailable(editorAvailable), _executableDirectory(std::move(executableDirectory)) {
	}

	ArgParseResult Parse(int argc, char *argv[]);

	const std::string &ProjectDirectory() const {
		return _projectDirectory;
	}

	AppMode Mode() const {
		return _mode;
	}

	bool ProjectIsZip() const {
		return _projectIsZip;
	}

	const BuildCommand &Build() const {
		return _build;
	}

	const DebugOptions &Debug() const {
		return _debug;
	}

  private:
	ArgParseResult parseBuild(const std::vector<std::string> &args);
	ArgParseResult findExportedArchive(const char *executable);
	ArgParseResult printHelp() const;
	ArgParseResult printVersion() const;
	ArgParseResult printLicenses() const;
	ArgParseResult printUsageError(const std::string &message) const;

  private:
	bool _editorAvailable = true;
	std::filesystem::path _executableDirectory;
	BuildCommand _build;
	DebugOptions _debug;
	std::string _projectDirectory = ".";
	AppMode _mode = AppMode::Run;
	bool _projectIsZip = false;
};

} // namespace sowa
