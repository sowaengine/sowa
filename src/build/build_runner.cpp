#include "build/build_runner.hpp"

#include <algorithm>
#include <fstream>
#include <random>
#include <set>
#include <sstream>

#include <fmt/format.h>

#include "build/icns.hpp"
#include "core/executable_path.hpp"
#include "core/filesystem/file_system.hpp"
#include "core/filesystem/zip_builder.hpp"
#include "core/log.hpp"
#include "core/scope_exit.hpp"
#include "core/serialize/archive.hpp"

namespace sowa::build {
namespace {

namespace fs = std::filesystem;

// What a previous build of a target wrote, so the next one can replace
// exactly those files and nothing else. Lives in the project, not the output.
struct ExportManifest : Serializable {
	std::string output;
	std::vector<std::string> files;

	void Serialize(Archive &archive) override {
		archive.Field("Output", output);
		archive.List("Files", files,
			[](Archive &element, std::string &file) { element.Value(file); });
	}
};

fs::path manifestPath(const fs::path &root, const std::string &target) {
	return root / ".sowa" / "exports" / (target + ".manifest");
}

bool readText(const fs::path &path, std::string &text) {
	std::ifstream file(path, std::ios::binary);
	if (!file) {
		return false;
	}
	std::stringstream buffer;
	buffer << file.rdbuf();
	text = buffer.str();
	return true;
}

bool writeText(const fs::path &path, std::string_view text, std::string &error) {
	std::error_code ec;
	fs::create_directories(path.parent_path(), ec);
	std::ofstream file(path, std::ios::binary | std::ios::trunc);
	file.write(text.data(), static_cast<std::streamsize>(text.size()));
	if (!file) {
		error = "cannot write " + path.string();
		return false;
	}
	return true;
}

bool copyFile(const fs::path &from, const fs::path &to, std::string &error) {
	std::error_code ec;
	fs::create_directories(to.parent_path(), ec);
	fs::copy_file(from, to, fs::copy_options::overwrite_existing, ec);
	if (ec) {
		error = "cannot copy " + from.string() + " to " + to.string() + ": " + ec.message();
		return false;
	}
	return true;
}

// Every regular file below root, relative and with '/' separators.
std::vector<std::string> listFiles(const fs::path &root) {
	std::vector<std::string> files;
	std::error_code ec;
	for (auto it = fs::recursive_directory_iterator(root, ec);
		!ec && it != fs::recursive_directory_iterator(); it.increment(ec)) {
		if (it->is_regular_file(ec)) {
			files.push_back(it->path().lexically_relative(root).generic_string());
		}
	}
	std::sort(files.begin(), files.end());
	return files;
}

bool isInside(const fs::path &child, const fs::path &parent) {
	const auto relative = child.lexically_relative(parent);
	return !relative.empty() && *relative.begin() != "..";
}

std::string htmlEscape(std::string_view text) {
	std::string out;
	for (const char c : text) {
		switch (c) {
		case '&':
			out += "&amp;";
			break;
		case '<':
			out += "&lt;";
			break;
		case '>':
			out += "&gt;";
			break;
		case '"':
			out += "&quot;";
			break;
		default:
			out += c;
		}
	}
	return out;
}

// The web template's page with the game's title and, when there is one, its
// icon as the favicon.
std::string patchWebPage(std::string html, const std::string &title, const std::string &favicon) {
	const auto open = html.find("<title>");
	const auto close = html.find("</title>");
	if (open != std::string::npos && close != std::string::npos && close > open) {
		html.replace(open + 7, close - open - 7, htmlEscape(title));
	}
	if (!favicon.empty()) {
		const auto head = html.find("</head>");
		if (head != std::string::npos) {
			html.insert(head, "<link rel=\"icon\" href=\"" + htmlEscape(favicon) + "\">\n");
		}
	}
	return html;
}

std::string xmlEscape(std::string_view text) {
	std::string out = htmlEscape(text);
	std::string escaped;
	for (const char c : out) {
		escaped += c == '\'' ? std::string("&apos;") : std::string(1, c);
	}
	return escaped;
}

// Reverse-DNS identifier macOS uses to tell apps apart (preferences, window
// state). Derived from the name until projects can set one.
std::string bundleIdentifier(std::string_view name) {
	std::string id;
	for (const char c : name) {
		if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-') {
			id += c;
		} else if (c >= 'A' && c <= 'Z') {
			id += static_cast<char>(c - 'A' + 'a');
		} else if (!id.empty() && id.back() != '-') {
			id += '-';
		}
	}
	while (!id.empty() && id.back() == '-') {
		id.pop_back();
	}
	return "com.sowa." + (id.empty() ? std::string("game") : id);
}

std::string infoPlist(const std::string &executable, const ProjectSettings &project, bool hasIcon) {
	const std::string version =
		project.application.version.empty() ? "0.1.0" : project.application.version;
	return fmt::format(R"(<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
	<key>CFBundleExecutable</key>
	<string>{0}</string>
	<key>CFBundleIdentifier</key>
	<string>{1}</string>
	<key>CFBundleName</key>
	<string>{2}</string>
	<key>CFBundleDisplayName</key>
	<string>{2}</string>
	<key>CFBundlePackageType</key>
	<string>APPL</string>
	<key>CFBundleShortVersionString</key>
	<string>{3}</string>
	<key>CFBundleVersion</key>
	<string>{3}</string>
	<key>CFBundleInfoDictionaryVersion</key>
	<string>6.0</string>
	<key>NSHighResolutionCapable</key>
	<true/>
{4}</dict>
</plist>
)",
		xmlEscape(executable), xmlEscape(bundleIdentifier(project.application.name)),
		xmlEscape(project.application.name.empty() ? executable : project.application.name),
		xmlEscape(version),
		hasIcon ? "\t<key>CFBundleIconFile</key>\n\t<string>AppIcon</string>\n" : "");
}

std::string randomSuffix() {
	std::random_device device;
	return fmt::format("{:08x}", device());
}

class TargetBuild {
  public:
	TargetBuild(BuildScript &script, BuildTarget &target, const fs::path &root,
		const fs::path &templates)
		: _script(script), _target(target), _root(root), _templates(templates) {
	}

	bool Run(std::string &error) {
		if (!_script.Configure(_target, error)) {
			return false;
		}

		std::vector<std::string> warnings;
		const auto resFiles = ResolveRules(_target.resRules, _target.resolve, warnings, error);
		if (!resFiles) {
			error = "ctx:mount(\"res\"): " + error;
			return false;
		}
		const auto outputFiles =
			ResolveRules(_target.outputRules, _target.resolve, warnings, error);
		if (!outputFiles) {
			error = "ctx:output_dir(): " + error;
			return false;
		}
		for (const std::string &warning : warnings) {
			Log::Warn("[{}] {}", _target.name, warning);
		}

		_staging =
			fs::temp_directory_path() / ("sowa-build-" + _target.name + "-" + randomSuffix());
		ScopeExit cleanup([this] {
			std::error_code ec;
			fs::remove_all(_staging, ec);
		});
		const fs::path res = _staging / "res";
		const fs::path out = _staging / "out";

		if (!stage(*resFiles, res, error) || !stage(*outputFiles, out, error)) {
			return false;
		}
		Archive archive = Archive::Save();
		_target.project.Serialize(archive);
		if (!writeText(res / "project.sowa", archive.ToString(), error)) {
			return false;
		}

		_target.phase = BuildPhase::Staged;
		if (!_script.Emit("staged", _target, error)) {
			return false;
		}

		_target.phase = BuildPhase::Stage;
		if (!pack(res, out, *outputFiles, error)) {
			return false;
		}
		if (!install(out, error)) {
			return false;
		}
		_target.phase = BuildPhase::Done;
		return _script.Emit("done", _target, error);
	}

  private:
	bool stage(const std::vector<ResolvedFile> &files, const fs::path &into, std::string &error) {
		std::error_code ec;
		fs::create_directories(into, ec);
		for (const ResolvedFile &file : files) {
			const fs::path destination = into / file.destination;
			switch (file.source) {
			case ResolvedFile::Source::File:
				if (!copyFile(file.file, destination, error)) {
					return false;
				}
				break;
			case ResolvedFile::Source::Content:
				if (!writeText(destination, file.content, error)) {
					return false;
				}
				break;
			case ResolvedFile::Source::Function: {
				std::string bytes;
				if (!_script.RunWriteFunction(_target, file.function, bytes, error)) {
					error = "write_fn(\"" + file.destination + "\"): " + error;
					return false;
				}
				if (!writeText(destination, bytes, error)) {
					return false;
				}
				break;
			}
			}
		}
		return true;
	}

	bool pack(const fs::path &res, const fs::path &out, const std::vector<ResolvedFile> &extra,
		std::string &error) {
		const bool web = _target.platform == "web";
		const bool macos = _target.platform == "macos";
		const std::string name = SanitizeAppName(_target.project.application.name);
		// macOS games ship as <Name>.app so Finder opens them without a
		// Terminal window; the archive goes in its Resources directory.
		const std::string bundle = name + ".app";
		const std::string archivePath = web		? "game.zip"
										: macos ? bundle + "/Contents/Resources/" + name + ".zip"
												: name + ".zip";

		// Files the template provides, mapped to their paths in the output.
		std::vector<std::pair<std::string, std::string>> templateFiles;
		if (web) {
			templateFiles = {{"sowa.js", "sowa.js"}, {"sowa.wasm", "sowa.wasm"}};
		} else if (_target.platform == "windows") {
			templateFiles = {{"sowa.exe", name + ".exe"}};
		} else if (macos) {
			templateFiles = {{"sowa", bundle + "/Contents/MacOS/" + name}};
		} else {
			templateFiles = {{"sowa", name}};
		}

		std::set<std::string> reserved = {macos ? bundle : archivePath};
		for (const auto &[from, to] : templateFiles) {
			(void)from;
			reserved.insert(macos ? bundle : to);
		}
		std::string favicon;
		const std::string &icon = _target.project.application.icon;
		if (web) {
			reserved.insert("index.html");
			if (!icon.empty()) {
				favicon = "favicon" + fs::path(icon).extension().string();
				reserved.insert(favicon);
			}
		}
		for (const ResolvedFile &file : extra) {
			const std::string top = file.destination.substr(0, file.destination.find('/'));
			if (reserved.count(file.destination) || (macos && top == bundle)) {
				error = "ctx:output_dir(): '" + file.destination +
						"' is a file the export itself writes";
				return false;
			}
		}

		std::error_code ec;
		fs::create_directories((out / archivePath).parent_path(), ec);
		const FsError zipped = ZipBuilder().AddDirectory(res).Build(out / archivePath);
		if (zipped != FsError::Ok) {
			error = "cannot write " + archivePath + ": " + FsErrorToString(zipped);
			return false;
		}
		if (macos) {
			const bool hasIcon = writeAppIcon(out / bundle / "Contents" / "Resources", error);
			if (!error.empty() || !writeText(out / bundle / "Contents" / "Info.plist",
									  infoPlist(name, _target.project, hasIcon), error)) {
				return false;
			}
		}

		const fs::path templateDir = _templates / _target.platform;
		for (const auto &[from, to] : templateFiles) {
			if (!copyFile(templateDir / from, out / to, error)) {
				return false;
			}
		}
		if (!web) {
			fs::permissions(out / templateFiles.front().second,
				fs::perms::owner_exec | fs::perms::group_exec | fs::perms::others_exec,
				fs::perm_options::add, ec);
			return true;
		}

		if (!favicon.empty()) {
			std::string iconPath = icon.rfind("res://", 0) == 0 ? icon.substr(6) : icon;
			std::error_code ec;
			if (fs::is_regular_file(_root / iconPath, ec)) {
				if (!copyFile(_root / iconPath, out / favicon, error)) {
					return false;
				}
			} else {
				Log::Warn("[{}] icon '{}' does not exist; exporting without a favicon",
					_target.name, icon);
				favicon.clear();
			}
		}
		std::string html;
		if (!readText(templateDir / "sowa.html", html)) {
			error = "cannot read " + (templateDir / "sowa.html").string();
			return false;
		}
		return writeText(out / "index.html",
			patchWebPage(html, _target.project.application.name, favicon), error);
	}

	// The project icon as Resources/AppIcon.icns. A missing or undecodable
	// icon is a warning and returns false with error left empty.
	bool writeAppIcon(const fs::path &resources, std::string &error) {
		const std::string &icon = _target.project.application.icon;
		if (icon.empty()) {
			return false;
		}
		const std::string iconPath = icon.rfind("res://", 0) == 0 ? icon.substr(6) : icon;
		std::string image;
		if (!readText(_root / iconPath, image)) {
			Log::Warn("[{}] icon '{}' does not exist; exporting without an app icon", _target.name,
				icon);
			return false;
		}
		std::string problem;
		const auto icns = MakeIcns(image, problem);
		if (!icns) {
			Log::Warn("[{}] icon '{}': {}; exporting without an app icon", _target.name, icon,
				problem);
			return false;
		}
		return writeText(resources / "AppIcon.icns", *icns, error);
	}

	// Replaces the files this target wrote last time with the new ones,
	// refusing to overwrite anything it did not write itself.
	bool install(const fs::path &staged, std::string &error) {
		const fs::path &output = _target.output;
		const fs::path manifestFile = manifestPath(_root, _target.name);

		std::set<std::string> ours;
		std::string text;
		if (readText(manifestFile, text)) {
			ExportManifest previous;
			Archive archive = Archive::Load(text);
			previous.Serialize(archive);
			if (archive.IsValid() && previous.output == output.string()) {
				ours.insert(previous.files.begin(), previous.files.end());
			}
		}

		const std::vector<std::string> files = listFiles(staged);
		std::vector<std::string> conflicts;
		std::error_code ec;
		for (const std::string &file : files) {
			if (fs::exists(fs::symlink_status(output / file, ec)) && !ours.count(file)) {
				conflicts.push_back(file);
			}
		}
		if (!conflicts.empty()) {
			error = fmt::format("{} already contains files this build did not create: {}",
				output.string(), conflicts.front());
			for (std::size_t i = 1; i < std::min<std::size_t>(conflicts.size(), 5); ++i) {
				error += ", " + conflicts[i];
			}
			if (conflicts.size() > 5) {
				error += fmt::format(" (and {} more)", conflicts.size() - 5);
			}
			error += ". Remove them or choose another output directory.";
			return false;
		}

		// Claim old and new files before touching the output, so a copy that
		// fails halfway leaves files the next build may still replace.
		std::set<std::string> claimed = ours;
		claimed.insert(files.begin(), files.end());
		if (!writeManifest(manifestFile, output, {claimed.begin(), claimed.end()}, error)) {
			return false;
		}

		for (const std::string &file : ours) {
			fs::remove(output / file, ec);
			for (fs::path parent = (output / file).parent_path();
				isInside(parent, output) && fs::is_empty(parent, ec);
				parent = parent.parent_path()) {
				fs::remove(parent, ec);
			}
		}
		for (const std::string &file : files) {
			if (!copyFile(staged / file, output / file, error)) {
				return false;
			}
		}

		return writeManifest(manifestFile, output, files, error);
	}

	static bool writeManifest(const fs::path &path, const fs::path &output,
		std::vector<std::string> files, std::string &error) {
		ExportManifest manifest;
		manifest.output = output.string();
		manifest.files = std::move(files);
		Archive archive = Archive::Save();
		manifest.Serialize(archive);
		return writeText(path, archive.ToString(), error);
	}

	BuildScript &_script;
	BuildTarget &_target;
	fs::path _root;
	fs::path _templates;
	fs::path _staging;
};

} // namespace

fs::path DefaultTemplatesDirectory() {
	const fs::path executable = ExecutablePath();
	return executable.empty() ? fs::path("templates") : executable.parent_path() / "templates";
}

std::vector<std::string> TemplateFiles(std::string_view platform) {
	if (platform == "web") {
		return {"sowa.html", "sowa.js", "sowa.wasm"};
	}
	if (platform == "windows") {
		return {"sowa.exe"};
	}
	return {"sowa"};
}

std::string SanitizeAppName(std::string_view applicationName) {
	std::string name(applicationName);
	for (char &c : name) {
		if (static_cast<unsigned char>(c) < 32 ||
			std::string_view("/\\:*?\"<>|").find(c) != std::string_view::npos) {
			c = '_';
		}
	}
	if (name.empty() || name == "." || name == "..") {
		name = "Game";
	}
	return name;
}

std::string ValidateExportTarget(const BuildScript &script, const ExportTargets &targets,
	const ExportTarget &target, const fs::path &projectRoot,
	const std::vector<std::pair<std::string, std::string>> &overrides) {
	if (std::string problem = ValidateExportTargetShape(targets, target); !problem.empty()) {
		return problem;
	}
	const auto &platforms = script.Platforms();
	if (std::find(platforms.begin(), platforms.end(), target.platform) == platforms.end()) {
		return "Target '" + target.name + "': build.lua does not support platform '" +
			   target.platform + "'.";
	}
	std::string error;
	if (!ResolveParams(script.Params(), target.params, overrides, projectRoot, error)) {
		return "Target '" + target.name + "': " + error + ".";
	}
	return {};
}

BuildReport RunBuild(const BuildRequest &request) {
	BuildReport report;
	std::error_code ec;
	const fs::path root = fs::canonical(request.projectRoot, ec);
	if (ec || !fs::is_regular_file(root / "project.sowa", ec)) {
		report.error = "No project.sowa in " + request.projectRoot.string();
		return report;
	}

	FileSystem files;
	files.MountFolder("res", root);
	ProjectSettings project;
	if (!project.Load(files)) {
		report.error = "Cannot load project.sowa";
		return report;
	}

	BuildScript script;
	if (!script.Load(root, report.error)) {
		return report;
	}
	ExportTargets targets;
	if (!targets.Load(root, report.error)) {
		return report;
	}

	std::vector<const ExportTarget *> selected;
	if (request.all) {
		if (targets.targets.empty()) {
			report.error = "export_targets.sowa has no targets. Add one in the editor's Export "
						   "dialog.";
			return report;
		}
		for (const ExportTarget &target : targets.targets) {
			selected.push_back(&target);
		}
	} else {
		for (const std::string &name : request.targets) {
			const ExportTarget *target = targets.Find(name);
			if (target == nullptr) {
				report.error = "No target named '" + name + "' in export_targets.sowa.";
				return report;
			}
			if (std::find(selected.begin(), selected.end(), target) == selected.end()) {
				selected.push_back(target);
			}
		}
	}

	const fs::path templates = request.templatesDirectory.empty()
								   ? DefaultTemplatesDirectory()
								   : fs::absolute(request.templatesDirectory);

	// Everything that can fail without building is checked for every target
	// before the first one starts.
	std::vector<std::string> excluded = {std::string(kBuildScriptFile),
		std::string(kExportTargetsFile), ".sowa", "export"};
	for (const ExportTarget &target : targets.targets) {
		const fs::path output = ExportTargetOutput(root, target);
		if (isInside(output, root)) {
			excluded.push_back(output.lexically_relative(root).generic_string());
		}
	}
	std::set<fs::path> outputs;
	for (const ExportTarget *target : selected) {
		if (std::string problem =
				ValidateExportTarget(script, targets, *target, root, request.paramOverrides);
			!problem.empty()) {
			report.error = problem;
			return report;
		}
		for (const std::string &file : TemplateFiles(target->platform)) {
			if (!fs::is_regular_file(templates / target->platform / file, ec)) {
				report.error = fmt::format("Target '{}': export template {} is missing. Build "
										   "the {} template, or pass --templates.",
					target->name, (templates / target->platform / file).string(), target->platform);
				return report;
			}
		}
		const fs::path output = ExportTargetOutput(root, *target);
		if (output == root || isInside(root, output)) {
			report.error = "Target '" + target->name + "': the output directory " +
						   output.string() + " contains the project.";
			return report;
		}
		if (fs::exists(output, ec) && !fs::is_directory(output, ec)) {
			report.error = "Target '" + target->name + "': " + output.string() +
						   " exists and is not a directory.";
			return report;
		}
		if (!outputs.insert(output).second) {
			report.error = "Target '" + target->name +
						   "' exports into the same directory as "
						   "another selected target: " +
						   output.string();
			return report;
		}
	}

	for (const ExportTarget *selectedTarget : selected) {
		BuildTarget target;
		target.name = selectedTarget->name;
		target.platform = selectedTarget->platform;
		target.params = *ResolveParams(script.Params(), selectedTarget->params,
			request.paramOverrides, root, report.error);
		target.project = project;
		target.output = ExportTargetOutput(root, *selectedTarget);
		target.resolve.projectRoot = root;
		target.resolve.excluded = excluded;

		Log::Info("Building '{}' ({}) -> {}", target.name, target.platform, target.output.string());
		TargetBuild build(script, target, root, templates);
		std::string error;
		if (!build.Run(error)) {
			script.EmitFailed(target, error);
			report.error = "Target '" + target.name + "': " + error;
			return report;
		}
		Log::Info("Built '{}' -> {}", target.name, target.output.string());
		report.built.emplace_back(target.name, target.output);
	}
	report.ok = true;
	return report;
}

std::string DescribeBuildProject(const fs::path &projectRoot) {
	std::error_code ec;
	const fs::path root = fs::canonical(projectRoot, ec);
	if (ec) {
		return "Cannot open " + projectRoot.string() + "\n";
	}
	std::string out;
	BuildScript script;
	std::string error;
	if (!script.Load(root, error)) {
		return "build.lua: " + error + "\n";
	}
	out += script.HasBuildFile() ? "build.lua\n" : "build.lua (none; using the default)\n";
	out += "  platforms:";
	for (const std::string &platform : script.Platforms()) {
		out += " " + platform;
	}
	out += "\n  params:";
	if (script.Params().empty()) {
		out += " none";
	}
	out += "\n";
	for (const ParamDecl &param : script.Params()) {
		out += fmt::format("    {} ({}", param.name, ParamTypeName(param.type));
		if (param.type == ParamType::Enum) {
			std::string choices;
			for (const std::string &choice : param.choices) {
				choices += choices.empty() ? choice : "|" + choice;
			}
			out += ": " + choices;
		}
		out += ")";
		if (param.min) {
			out += fmt::format(" min={}", *param.min);
		}
		if (param.max) {
			out += fmt::format(" max={}", *param.max);
		}
		if (param.required) {
			out += " required";
		} else if (!std::holds_alternative<std::monostate>(param.defaultValue)) {
			out += " default=" + ParamValueToString(param.defaultValue);
		}
		if (!param.description.empty()) {
			out += " - " + param.description;
		}
		out += "\n";
	}

	ExportTargets targets;
	if (!targets.Load(root, error)) {
		return out + error + "\n";
	}
	out += "targets:";
	if (targets.targets.empty()) {
		out += " none";
	}
	out += "\n";
	for (const ExportTarget &target : targets.targets) {
		out += fmt::format("  {} ({}) -> {}\n", target.name, target.platform,
			ExportTargetOutput(root, target).string());
		for (const auto &[name, value] : target.params) {
			out += fmt::format("    {} = {}\n", name, value);
		}
		if (std::string problem = ValidateExportTarget(script, targets, target, root);
			!problem.empty()) {
			out += "    problem: " + problem + "\n";
		}
	}
	return out;
}

} // namespace sowa::build
