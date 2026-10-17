#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "build/glob.hpp"

namespace sowa::build {

// One call on a builder (ctx:mount("res") / ctx:output_dir()), kept in call
// order. Later rules win over earlier ones for the same destination.
struct FileRule {
	enum class Kind {
		Include,
		Exclude,
		Write,
		WriteFunction,
	};

	Kind kind = Kind::Include;
	// Include / Exclude.
	std::optional<GlobPattern> pattern;
	// Include only: rename (explicit patterns) or re-root under a directory
	// ("." is the builder root). At most one is set.
	std::optional<std::string> as;
	std::optional<std::string> into;
	// Write / WriteFunction: the destination path.
	std::string path;
	// Write: the file contents.
	std::string content;
	// WriteFunction: index into the script's write functions.
	int function = -1;
};

// A file the builder will produce, after all rules are applied.
struct ResolvedFile {
	enum class Source {
		File,
		Content,
		Function,
	};

	std::string destination;
	Source source = Source::File;
	std::filesystem::path file;
	std::string content;
	int function = -1;
};

struct ResolveContext {
	// Canonical project directory; patterns are relative to it.
	std::filesystem::path projectRoot;
	// Project-relative files and directories no rule can pick up (build.lua,
	// export_targets.sowa, .sowa, output directories inside the project).
	std::vector<std::string> excluded;
};

// Applies rules in order. Explicit includes that match nothing, symlinks
// leaving the project and directories named explicitly are errors; globs
// matching nothing add a warning. Result is sorted by destination.
std::optional<std::vector<ResolvedFile>> ResolveRules(const std::vector<FileRule> &rules,
	const ResolveContext &context, std::vector<std::string> &warnings, std::string &error);

} // namespace sowa::build
