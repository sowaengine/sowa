#ifndef SW_RUNNER_COMMANDS_HPP_
#define SW_RUNNER_COMMANDS_HPP_

#include <string>
#include <vector>
#include <filesystem>

void CommandUpdate(std::filesystem::path appPath, bool showMessagebox=false);
void CommandVersion();
void CommandList(std::filesystem::path appPath, bool isLocal);
void CommandHelp();
void RunVersion(std::string version, std::filesystem::path appPath, std::vector<std::string> args);
void ActivateVersion(std::string version);
void InstallVersion(std::string version, std::filesystem::path appPath);

#endif