#ifndef SW_RUNNER_UTILS_HPP_
#define SW_RUNNER_UTILS_HPP

#include <vector>
#include <string>
#include "config.hpp"

std::vector<std::string> Split(std::string text, const std::string &delimiter);

std::vector<std::string> GetAllVersions(std::string url);

std::vector<Version> ParseVersions(std::string yamlDoc);

#endif