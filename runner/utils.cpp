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
	std::string content = GetContentHTTP(url);
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

/*
    1.0  -----------> 1.0.0-stable
    0.1.1-a1-65809 -> 0.1.1-a1
*/
std::string ResolveVersion(std::string version) {
    auto tokens = Split(version, ".");
    if(tokens.size() == 1) {
        return version + ".0.0-stable";
    }
    if(tokens.size() == 2) {
        return version + ".0-stable";
    }
    if(tokens.size() == 3) {
        auto rhsTokens = Split(tokens[2], "-");
        if(rhsTokens.size() == 1) {
            return version + "-stable";
        }
        if(rhsTokens.size() == 3) {
            //       0          .       1         .         1          .       a1
            return tokens[0] + "." + tokens[1] + "." + rhsTokens[0] + "-" + rhsTokens[1];
        }
    }
    
    return version;
}


void GetVersionsFromServer() {
    for(const VersionServer& s : GetConfig().servers) {
        std::string content = GetContentHTTP(s.url);
        std::vector<Version> versions = ParseVersions(content);
        for(const Version& v : versions) {
            GetRuntimeConfig().versions[s.name].push_back(v);
        }
    }
}