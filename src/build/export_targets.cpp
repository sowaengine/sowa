#include "build/export_targets.hpp"

#include <algorithm>
#include <fstream>
#include <sstream>

namespace sowa::build {

const std::vector<std::string> &KnownPlatforms() {
	static const std::vector<std::string> platforms = {"windows", "macos", "linux", "web"};
	return platforms;
}

bool IsKnownPlatform(std::string_view platform) {
	const auto &platforms = KnownPlatforms();
	return std::find(platforms.begin(), platforms.end(), platform) != platforms.end();
}

std::string HostPlatform() {
#if defined(_WIN32)
	return "windows";
#elif defined(__APPLE__)
	return "macos";
#elif defined(__EMSCRIPTEN__)
	return "web";
#else
	return "linux";
#endif
}

void ExportTargets::Serialize(Archive &archive) {
	archive.List("Targets", targets, [](Archive &element, ExportTarget &target) {
		element.Field("Name", target.name);
		element.Field("Platform", target.platform);
		if (element.IsLoading() || !target.output.empty()) {
			element.Field("Output", target.output);
		}
		if (element.IsSaving() && target.params.empty()) {
			return;
		}
		Archive params = element.Child("Params");
		if (element.IsSaving()) {
			for (auto &[name, value] : target.params) {
				params.Field(name, value);
			}
			return;
		}
		target.params.clear();
		for (const std::string &name : params.Keys()) {
			std::string value;
			if (params.Field(name, value)) {
				target.params.emplace_back(name, value);
			}
		}
	});
}

bool ExportTargets::Load(const std::filesystem::path &projectRoot, std::string &error) {
	targets.clear();
	const auto path = projectRoot / kExportTargetsFile;
	std::error_code ec;
	if (!std::filesystem::exists(path, ec)) {
		return true;
	}
	std::ifstream file(path, std::ios::binary);
	if (!file) {
		error = "Cannot read " + path.string();
		return false;
	}
	std::stringstream text;
	text << file.rdbuf();
	Archive archive = Archive::Load(text.str());
	if (!archive.IsValid()) {
		error = std::string(kExportTargetsFile) + ": " +
				(archive.Errors().empty() ? "invalid YAML" : archive.Errors().front());
		return false;
	}
	Serialize(archive);
	if (archive.HasErrors()) {
		error = std::string(kExportTargetsFile) + ": " + archive.Errors().front();
		return false;
	}
	return true;
}

bool ExportTargets::Save(const std::filesystem::path &projectRoot, std::string &error) {
	Archive archive = Archive::Save();
	Serialize(archive);
	const auto path = projectRoot / kExportTargetsFile;
	std::ofstream file(path, std::ios::binary | std::ios::trunc);
	file << archive.ToString();
	if (!file) {
		error = "Cannot write " + path.string();
		return false;
	}
	return true;
}

ExportTarget *ExportTargets::Find(std::string_view name) {
	const auto it = std::find_if(targets.begin(), targets.end(),
		[&](const ExportTarget &target) { return target.name == name; });
	return it == targets.end() ? nullptr : &*it;
}

const ExportTarget *ExportTargets::Find(std::string_view name) const {
	return const_cast<ExportTargets *>(this)->Find(name);
}

std::string ValidateExportTargetShape(const ExportTargets &targets, const ExportTarget &target) {
	if (target.name.empty()) {
		return "Target names cannot be empty.";
	}
	for (const char c : target.name) {
		const bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
						(c >= '0' && c <= '9') || c == '_' || c == '-';
		if (!ok) {
			return "Target '" + target.name + "': names may only use letters, digits, '_' and '-'.";
		}
	}
	const auto count = std::count_if(targets.targets.begin(), targets.targets.end(),
		[&](const ExportTarget &other) { return other.name == target.name; });
	if (count > 1) {
		return "Target name '" + target.name + "' is used more than once.";
	}
	if (!IsKnownPlatform(target.platform)) {
		return "Target '" + target.name + "': unknown platform '" + target.platform + "'.";
	}
	return {};
}

std::filesystem::path ExportTargetOutput(const std::filesystem::path &projectRoot,
	const ExportTarget &target) {
	const std::filesystem::path output = target.output.empty()
											 ? std::filesystem::path("export") / target.name
											 : std::filesystem::path(target.output);
	return (output.is_absolute() ? output : projectRoot / output).lexically_normal();
}

} // namespace sowa::build
