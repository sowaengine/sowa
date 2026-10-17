#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "build/build_script.hpp"
#include "build/export_targets.hpp"

namespace sowa::build {

struct BuildRequest {
	std::filesystem::path projectRoot;
	// Names from export_targets.sowa, or all.
	std::vector<std::string> targets;
	bool all = false;
	// CLI --param overrides, applied to every selected target.
	std::vector<std::pair<std::string, std::string>> paramOverrides;
	// Empty: DefaultTemplatesDirectory().
	std::filesystem::path templatesDirectory;
};

struct BuildReport {
	bool ok = false;
	std::string error;
	// Output directory of every target that finished, in build order.
	std::vector<std::pair<std::string, std::filesystem::path>> built;
};

// Builds the requested targets in order, stopping at the first failure.
// Progress and warnings go to the log; the failure is only in the report.
BuildReport RunBuild(const BuildRequest &request);

// <directory of the running executable>/templates.
std::filesystem::path DefaultTemplatesDirectory();

// The files a platform's export template consists of, relative to
// <templates>/<platform>/.
std::vector<std::string> TemplateFiles(std::string_view platform);

// Turns an application name into a safe file name ("Game" if nothing is left).
std::string SanitizeAppName(std::string_view name);

// Checks a saved target against build.lua: declared platform, and params
// that parse (with overrides applied). Empty when the target is buildable.
std::string ValidateExportTarget(const BuildScript &script, const ExportTargets &targets,
	const ExportTarget &target, const std::filesystem::path &projectRoot,
	const std::vector<std::pair<std::string, std::string>> &overrides = {});

// sowa build --list output.
std::string DescribeBuildProject(const std::filesystem::path &projectRoot);

} // namespace sowa::build
