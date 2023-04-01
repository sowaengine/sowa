#include "utils.hpp"
#include "net.hpp"
#include "yaml-cpp/yaml.h"
#include <iostream>

std::vector<std::string> Split(std::string text, const std::string &delimiter) {
	std::vector<std::string> tokens;

	size_t pos = 0;
	std::string token;
	while ((pos = text.find(delimiter)) != std::string::npos) {
		token = text.substr(0, pos);
		tokens.push_back(token);
		text.erase(0, pos + delimiter.length());
	}
	tokens.push_back(text);
	return tokens;
}

std::vector<std::string> GetAllVersions(std::string url) {
	std::string content = GetFileHTTP(url);
	std::cout << "Got Content: " << content << std::endl;
	return {};
}

std::vector<Version> ParseVersions(std::string yamlDoc) {
	std::vector<Version> versions{};

	YAML::Node node = YAML::Load(yamlDoc);
	YAML::Node vers = node["Versions"];
	if (vers) {
		for (YAML::const_iterator it = vers.begin(); it != vers.end(); ++it) {
			std::string key = it->first.as<std::string>("");
			YAML::Node value = it->second;

            #if defined(SW_LINUX)
                std::string url = value["Url"]["Linux"].as<std::string>("");
            #elif defined(SW_WINDOWS)
                std::string url = value["Url"]["Win64"].as<std::string>("");
            #endif

            versions.push_back(Version(key, url));
		}
	}

	return versions;
}