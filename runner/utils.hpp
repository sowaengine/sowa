#ifndef SW_RUNNER_UTILS_HPP_
#define SW_RUNNER_UTILS_HPP_

#include "config.hpp"
#include <string>
#include <vector>

struct BranchVersionPair {
	std::string branch;
	std::string version;

	// branch/version
	std::string Repr() { return branch + "/" + version; }

	BranchVersionPair(const std::string &branch_, const std::string &version_) : branch(branch_), version(version_) {}
};

// {major}.{minor}.{patch}-{tag}{rev}
enum class VersionTag : uint32_t {
	alpha = 0,
	beta,
	rc,
	stable
};
struct VersionName {
	int major;
	int minor;
	int patch;
	VersionTag tag;
	int rev;

	VersionName();
	VersionName(std::string);

	std::string Repr();

	uint32_t Hash() const {
		return (
			((major % 256) << 24) |
			((minor % 256) << 16) |
			((patch % 256) << 8) |
			(0x1 << (4 + (int)tag % 4)) |
			(rev % 16));
	}
	bool operator<(const VersionName &o) {
		return Hash() < o.Hash();
	}
	bool operator==(const VersionName &o) {
		return Hash() == o.Hash();
	}
	bool operator>(const VersionName &o) {
		return Hash() > o.Hash();
	}
};

std::vector<std::string> Split(std::string text, const std::string &delimiter);

std::vector<std::string> GetAllVersions(std::string url);

std::vector<Version> ParseVersions(std::string yamlDoc);

/*
	1.0  -----------> 1.0.0-stable
	0.1.1-a1-65809 -> 0.1.1-a1
*/
std::string ResolveVersion(std::string version);

// Resolves main/latest
BranchVersionPair ResolveFullVersion(std::string version);

bool IsInteger(std::string number);

bool StartsWith(std::string text, std::string with);

void GetVersionsFromServer(const std::vector<VersionServer> &servers);

void InstallSowa(std::filesystem::path appPath, std::string url, BranchVersionPair ver, bool installRunner=true);

void TempFileCleanup(std::filesystem::path appPath);

#endif