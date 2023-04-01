#ifndef SW_RUNNER_FILESYSTEM_HPP_
#define SW_RUNNER_FILESYSTEM_HPP_

#include <filesystem>
#include <iostream>
#include <vector>
#include <string>

std::filesystem::path GetExecutableDir();
FILE* OpenExecutable(std::string path, std::vector<std::string> args);
std::string GetLatestVersion(std::filesystem::path binPath);
std::vector<std::filesystem::directory_entry> GetLocalVersions(std::filesystem::path binPath);

#endif