#ifndef SW_RUNNER_UTILS_HPP_
#define SW_RUNNER_UTILS_HPP

#include <vector>
#include <string>
#include "config.hpp"

std::vector<std::string> Split(std::string text, const std::string &delimiter);

std::vector<std::string> GetAllVersions(std::string url);

std::vector<Version> ParseVersions(std::string yamlDoc);

/*
    1.0  -----------> 1.0.0-stable
    0.1.1-a1-65809 -> 0.1.1-a1
*/
std::string ResolveVersion(std::string version);

void GetVersionsFromServer();

#endif