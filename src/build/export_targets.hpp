#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "core/serialize/archive.hpp"

namespace sowa::build {

inline constexpr std::string_view kExportTargetsFile = "export_targets.sowa";

// Platform.* values, in the order Platform lists them.
const std::vector<std::string> &KnownPlatforms();
bool IsKnownPlatform(std::string_view platform);
// The platform this binary runs on ("macos", "linux", "windows").
std::string HostPlatform();

// One named export configuration from export_targets.sowa.
struct ExportTarget {
	std::string name;
	std::string platform;
	// Relative to the project directory, or absolute. Empty: export/<name>.
	std::string output;
	// Param values as written in the file; typed against build.lua when built.
	std::vector<std::pair<std::string, std::string>> params;
};

// The whole export_targets.sowa document.
class ExportTargets : public Serializable {
  public:
	std::vector<ExportTarget> targets;

	void Serialize(Archive &archive) override;

	// A missing file loads as an empty list. A file that exists but cannot be
	// read or parsed fails with error.
	bool Load(const std::filesystem::path &projectRoot, std::string &error);
	bool Save(const std::filesystem::path &projectRoot, std::string &error);

	ExportTarget *Find(std::string_view name);
	const ExportTarget *Find(std::string_view name) const;
};

// Name rules ([A-Za-z0-9_-]+, unique) and a known platform. Does not check
// params; those need build.lua (see BuildScript).
std::string ValidateExportTargetShape(const ExportTargets &targets, const ExportTarget &target);

// The absolute output directory a target exports into.
std::filesystem::path ExportTargetOutput(const std::filesystem::path &projectRoot,
	const ExportTarget &target);

} // namespace sowa::build
