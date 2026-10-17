#include "core/arg_parser.hpp"

#include <charconv>
#include <cstdio>
#include <filesystem>
#include <string_view>
#include <system_error>
#include <vector>

#include "assets/embedded.hpp"
#include "core/build_info.hpp"
#include "core/executable_path.hpp"

namespace sowa {

namespace {

const char *kUsage = "Usage: sowa [command] [arguments]\n"
					 "\n"
					 "Without arguments, open Project Manager.\n"
					 "\n"
					 "Commands:\n"
					 "  project-manager Open Project Manager.\n"
					 "  edit [path]     Open a project folder in the editor.\n"
					 "                  Without a local project, open Project Manager.\n"
					 "  run [path]      Run a project directory or ZIP archive.\n"
					 "                  Without a path, use <executable-name>.zip next to the\n"
					 "                  executable if present, otherwise the current directory.\n"
					 "  build [path] (--target <name>... | --all) [--param <key>=<value>]...\n"
					 "               [--templates <dir>]\n"
					 "                  Export the project's targets from export_targets.sowa\n"
					 "                  using its build.lua. --param overrides a param on every\n"
					 "                  selected target for this run only.\n"
					 "  build [path] --list\n"
					 "                  List the platforms, params and targets of a project.\n"
					 "\n"
					 "Debugging options (run, edit):\n"
					 "  --debug-port <n>      Port VS Code attaches to (default 6510).\n"
					 "  --wait-for-debugger   Hold game scripts until VS Code has attached.\n"
					 "\n"
					 "Options:\n"
					 "  -h, --help      Show this help message and exit.\n"
					 "  -v, --version   Show version information and exit.\n"
					 "  --licenses      Show bundled font licenses and exit.\n";

const char *kTemplateUsage =
	"Usage: sowa [run [path]]\n"
	"\n"
	"Without arguments, run <executable-name>.zip next to the executable\n"
	"if present, otherwise the current directory.\n"
	"\n"
	"Commands:\n"
	"  run [path]      Run a project directory or ZIP archive.\n"
	"\n"
	"Debugging options (debug templates):\n"
	"  --debug-port <n>      Port VS Code attaches to (default 6510).\n"
	"  --wait-for-debugger   Hold game scripts until VS Code has attached.\n"
	"\n"
	"Options:\n"
	"  -h, --help      Show this help message and exit.\n"
	"  -v, --version   Show version information and exit.\n"
	"  --licenses      Show bundled font licenses and exit.\n";

} // namespace

ArgParseResult ArgParser::Parse(int argc, char *argv[]) {
	_projectDirectory = ".";
	_mode = AppMode::Run;
	_projectIsZip = false;
	_build = {};
	_debug = {};
	std::vector<std::string> args;
	for (int i = 1; i < argc; ++i) {
		// Older macOS versions pass a process serial number to apps opened
		// from Finder.
		if (std::string_view(argv[i]).rfind("-psn_", 0) != 0) {
			args.emplace_back(argv[i]);
		}
	}

	for (const std::string &arg : args) {
		if (arg == "-h" || arg == "--help") {
			return printHelp();
		}
		if (arg == "-v" || arg == "--version") {
			return printVersion();
		}
		if (arg == "--licenses") {
			return printLicenses();
		}
	}

	if (args.empty() && _editorAvailable) {
		_mode = AppMode::ProjectManager;
		return ArgParseResult::Continue;
	}
	// An export template has only run, so it may be left out before options
	// (game --wait-for-debugger).
	if (args.empty() || (!_editorAvailable && args[0].rfind("--", 0) == 0)) {
		args.insert(args.begin(), "run");
	}
	const std::string command = args[0];
	if (!_editorAvailable && command != "run") {
		return printUsageError("'" + command + "' is not available in export templates.");
	}
	if (command == "run") {
		_mode = AppMode::Run;
	} else if (command == "edit") {
		_mode = AppMode::Edit;
	} else if (command == "build") {
		_mode = AppMode::Build;
		return parseBuild(args);
	} else if (command == "project-manager") {
		if (args.size() != 1) {
			return printUsageError("The project-manager command takes no arguments.");
		}
		_mode = AppMode::ProjectManager;
		return ArgParseResult::Continue;
	} else {
		return printUsageError("Unknown command '" + command + "'.");
	}

	// Debugging options may appear anywhere after run/edit; the rest are
	// positional.
	std::vector<std::string> positional{args[0]};
	for (std::size_t i = 1; i < args.size(); ++i) {
		const std::string &arg = args[i];
		if (arg == "--wait-for-debugger") {
			_debug.waitForDebugger = true;
		} else if (arg == "--debug-port") {
			if (i + 1 >= args.size()) {
				return printUsageError("--debug-port expects a port number.");
			}
			const std::string &value = args[++i];
			unsigned int port = 0;
			const auto [end, error] =
				std::from_chars(value.data(), value.data() + value.size(), port);
			if (error != std::errc{} || end != value.data() + value.size() || port == 0 ||
				port > 65535) {
				return printUsageError("--debug-port expects a port number, got '" + value + "'.");
			}
			_debug.port = static_cast<std::uint16_t>(port);
		} else if (!arg.empty() && arg.front() == '-') {
			return printUsageError("Unknown option '" + arg + "' for '" + command + "'.");
		} else {
			positional.push_back(arg);
		}
	}
	args = std::move(positional);

	if (args.size() > 2) {
		return printUsageError("Too many arguments for '" + command + "'.");
	}
	_projectDirectory = args.size() == 2 ? args[1] : ".";
	if (_mode == AppMode::Run && args.size() < 2) {
		if (const ArgParseResult result = findExportedArchive(argv[0]);
			result != ArgParseResult::Continue) {
			return result;
		}
	}
	const auto extension = std::filesystem::path(_projectDirectory).extension();
	std::error_code ec;
	_projectIsZip = (extension == ".zip" || extension == ".ZIP") &&
					!std::filesystem::is_directory(_projectDirectory, ec);
	if (_mode == AppMode::Edit && _projectIsZip) {
		return printUsageError("ZIP projects are read-only; open a project directory to edit.");
	}
	// An edit command without a project opens Project Manager.
	if ((command == "edit" && args.size() == 1) && !_projectIsZip &&
		!std::filesystem::exists(std::filesystem::path(_projectDirectory) / "project.sowa", ec)) {
		_mode = AppMode::ProjectManager;
	}
	return ArgParseResult::Continue;
}

ArgParseResult ArgParser::parseBuild(const std::vector<std::string> &args) {
	bool haveProject = false;
	for (std::size_t i = 1; i < args.size(); ++i) {
		const std::string &arg = args[i];
		const bool takesValue = arg == "--target" || arg == "--param" || arg == "--templates";
		if (takesValue &&
			(i + 1 >= args.size() || args[i + 1].empty() || args[i + 1].front() == '-')) {
			return printUsageError("'" + arg + "' needs a value.");
		}
		if (arg == "--target") {
			_build.targets.push_back(args[++i]);
		} else if (arg == "--param") {
			const std::string &pair = args[++i];
			const auto equals = pair.find('=');
			if (equals == std::string::npos || equals == 0) {
				return printUsageError("--param expects <key>=<value>, got '" + pair + "'.");
			}
			_build.params.emplace_back(pair.substr(0, equals), pair.substr(equals + 1));
		} else if (arg == "--templates") {
			_build.templatesDirectory = args[++i];
		} else if (arg == "--all") {
			_build.all = true;
		} else if (arg == "--list") {
			_build.list = true;
		} else if (arg.rfind("-", 0) == 0) {
			return printUsageError("Unknown build option '" + arg + "'.");
		} else if (haveProject) {
			return printUsageError("Too many arguments for 'build'.");
		} else {
			_projectDirectory = arg;
			haveProject = true;
		}
	}

	if (_build.list) {
		if (_build.all || !_build.targets.empty() || !_build.params.empty() ||
			!_build.templatesDirectory.empty()) {
			return printUsageError("--list cannot be combined with other build options.");
		}
	} else if (_build.all && !_build.targets.empty()) {
		return printUsageError("--all cannot be combined with --target.");
	} else if (!_build.all && _build.targets.empty()) {
		return printUsageError("build needs either --target <name> or --all.");
	}
	return ArgParseResult::Continue;
}

ArgParseResult ArgParser::findExportedArchive(const char *executable) {
	// Find APP_NAME.zip beside the executable, independent of the working directory.
	std::filesystem::path directory = _executableDirectory;
	std::filesystem::path filename = std::filesystem::path(executable).filename();
	if (directory.empty()) {
		std::filesystem::path path = ExecutablePath();
		if (path.empty()) {
			std::error_code absoluteError;
			path = std::filesystem::absolute(executable, absoluteError);
		}
		directory = path.parent_path();
		filename = path.filename();
	}
	if (filename.extension() == ".exe") {
		filename.replace_extension();
	}
	auto archive = directory / (filename.string() + ".zip");
	std::error_code ec;
	// A macOS .app keeps it in Contents/Resources, next to Contents/MacOS.
	const auto bundled = directory.parent_path() / "Resources" / archive.filename();
	if (!std::filesystem::exists(archive, ec) && directory.filename() == "MacOS" &&
		std::filesystem::exists(bundled, ec)) {
		archive = bundled;
	}
	if (std::filesystem::exists(archive, ec)) {
		_projectDirectory = archive.string();
	}
	if (ec) {
		return printUsageError("Cannot inspect exported game archive: " + ec.message());
	}
	return ArgParseResult::Continue;
}

ArgParseResult ArgParser::printHelp() const {
	std::printf("%s", _editorAvailable ? kUsage : kTemplateUsage);
	return ArgParseResult::ExitSuccess;
}

ArgParseResult ArgParser::printVersion() const {
	std::printf("Sowa Engine %s (%s@%s%s)\n", BuildInfo::Version(), BuildInfo::GitHash(),
		BuildInfo::GitBranch(), BuildInfo::IsDirty() ? "-dirty" : "");
	return ArgParseResult::ExitSuccess;
}

ArgParseResult ArgParser::printLicenses() const {
	std::puts("Roboto Regular\n");
	const auto roboto = assets::DefaultFontLicense();
	std::fwrite(roboto.data(), 1, roboto.size(), stdout);
#if SOWA_EDITOR
	if (_editorAvailable) {
		std::puts("\nFont Awesome Free\n");
		const auto icons = assets::EditorIconsLicense();
		std::fwrite(icons.data(), 1, icons.size(), stdout);
	}
#endif
	return ArgParseResult::ExitSuccess;
}

ArgParseResult ArgParser::printUsageError(const std::string &message) const {
	std::fprintf(stderr, "Error: %s\n\n", message.c_str());
	std::fprintf(stderr, "%s", _editorAvailable ? kUsage : kTemplateUsage);
	return ArgParseResult::ExitFailure;
}

} // namespace sowa
