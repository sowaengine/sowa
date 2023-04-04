#ifndef SW_RUNNER_FILESYSTEM_HPP_
#define SW_RUNNER_FILESYSTEM_HPP_

#include <filesystem>
#include <iostream>
#include <vector>
#include <string>

std::filesystem::path GetExecutableDir();
FILE* OpenExecutable(std::string path, std::vector<std::string> args);

std::string GetLatestVersion(std::filesystem::path binPath);

/**
 * DOES NOT HANDLE LATEST VERSION
 * binpath: bin/main, version: 1.0 looks for bin/main/sowa-1.0.0...
*/
std::string GetVersionInPath(std::filesystem::path binPath, std::string version);
std::vector<std::filesystem::directory_entry> GetLocalVersions(std::filesystem::path binPath);

#endif