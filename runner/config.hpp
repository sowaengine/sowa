#ifndef SW_RUNNER_CONFIG_HPP_
#define SW_RUNNER_CONFIG_HPP_

#include <string>
#include <vector>
#include <filesystem>
#include <unordered_map>

struct VersionServer {
    std::string name;
    std::string url;

    VersionServer() {}
    VersionServer(std::string name, std::string url) : name(name), url(url) {}
};

struct RunnerConfig {
    std::string activeVersion;
    std::vector<VersionServer> servers;

    // AutoUpdate
    bool checkUpdates;
    uint64_t lastUpdateCheck;
    std::vector<std::string> activeBranches;
    uint32_t checkFrequency;
};
// appPath is used if ./runnerconfig.yml does not exitst
void LoadConfig(std::filesystem::path appPath);
void SaveConfig();

RunnerConfig& GetConfig();

struct Version {
    std::string tag;
    std::string url;

    Version() {}
    Version(std::string tag, std::string url) : tag(tag), url(url) {}
};

struct RuntimeConfig {
    // versions[server.name]
    std::unordered_map<std::string, std::vector<Version>> versions;
};
RuntimeConfig& GetRuntimeConfig();


#endif
