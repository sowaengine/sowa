#ifndef SW_RUNNER_UTILS_HPP_
#define SW_RUNNER_UTILS_HPP

#include <vector>
#include <string>
#include "config.hpp"

struct BranchVersionPair {
    std::string branch;
    std::string version;

    // branch/version
    std::string Repr() { return branch + "/" + version; }

    BranchVersionPair(const std::string& branch_, const std::string& version_) : branch(branch_), version(version_) {}
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

void GetVersionsFromServer();

void InstallSowa(std::filesystem::path appPath, std::string url, BranchVersionPair ver);

#endif