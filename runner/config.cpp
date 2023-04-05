#include "./config.hpp"
#include "yaml-cpp/yaml.h"
#include <fstream>
#include <iostream>
#include "console.hpp"

#include "./utils.hpp"

static RunnerConfig cfg{};
static RuntimeConfig runtimeCfg{};

static std::filesystem::path s_ConfigPath;

void LoadConfig(std::filesystem::path appPath) {
	YAML::Node config;
	try {
		config = YAML::LoadFile("./runnerconfig.yml");
		s_ConfigPath = "./runnerconfig.yml";
	} catch (const std::exception &e) {
		try {
			config = YAML::LoadFile((appPath / "runnerconfig.yml").string());
			s_ConfigPath = (appPath / "runnerconfig.yml");
		} catch (const std::exception &e) {
			std::cerr << "Failed to load config file!\n" << RED << e.what() << RESET << '\n';
			exit(0);
		}
	}

	cfg.activeVersion = config["ActiveVersion"].as<std::string>("latest");

	YAML::Node servers = config["Servers"];
	for (YAML::const_iterator it = servers.begin(); it != servers.end(); ++it) {
		std::string key = it->first.as<std::string>("");
		std::string value = it->second.as<std::string>("");
		cfg.servers.push_back(VersionServer(key, value));

		runtimeCfg.versions[key] = {};
	}

	YAML::Node autoUpdate = config["AutoUpdate"];
	cfg.checkUpdates = autoUpdate["CheckUpdates"].as<bool>(false);
	cfg.lastUpdateCheck = autoUpdate["LastUpdateCheck"].as<uint64_t>(0);
	cfg.activeBranches = autoUpdate["ActiveBranches"].as<std::vector<std::string>>(std::vector<std::string>{"main"});
	cfg.checkFrequency = autoUpdate["CheckFrequency"].as<uint32_t>(0);
}
void SaveConfig() {
	if(s_ConfigPath.string() == "") {
		std::cout << "Failed to save config file" << std::endl;
		return;
	}
	YAML::Node doc;

	doc["ActiveVersion"] = cfg.activeVersion;
	YAML::Node servers;
	for (auto s : cfg.servers) {
		servers[s.name] = s.url;
	}
	doc["Servers"] = servers;

	YAML::Node autoUpdate;
	autoUpdate["CheckUpdates"] = cfg.checkUpdates;
	autoUpdate["LastUpdateCheck"] = cfg.lastUpdateCheck;
	autoUpdate["ActiveBranches"] = cfg.activeBranches;
	autoUpdate["CheckFrequency"] = cfg.checkFrequency;
	doc["AutoUpdate"] = autoUpdate;

	YAML::Emitter emitter;
	emitter << doc;
	std::ofstream ofstream(s_ConfigPath.string().c_str());
	ofstream << emitter.c_str();
	ofstream.close();
}

RunnerConfig &GetConfig() {
	return cfg;
}

RuntimeConfig &GetRuntimeConfig() {
	return runtimeCfg;
}

bool Version::operator<(const Version& o) {
	return VersionName(tag) < VersionName(o.tag);
}