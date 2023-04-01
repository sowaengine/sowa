#include <iostream>
#include "../filesystem.hpp"
#include "../net.hpp"
#include "../config.hpp"
#include "../utils.hpp"
#include "../console.hpp"

void CommandList(std::filesystem::path appPath, bool isLocal) {
	if(isLocal) {
		auto versions = GetLocalVersions((appPath / "bin").string());

		std::cout << "Available sowa versions:\n";
		for (size_t i = 0; i < versions.size(); i++) {
			std::cout << "\t" << i << ". " << versions[i].path().parent_path().filename().string() << "/" << versions[i].path().filename().string() << "\n";
		}
		std::cout << "runner --run {version}\n";
		std::cout << "runner --activate {version}\n";
		std::cout << std::endl;
	} else {
		std::cout << "Available sowa versions:\n";
		for(const VersionServer& s : GetConfig().servers) {
			std::string content = GetFileHTTP(s.url);
			std::vector<Version> versions = ParseVersions(content);
			for(const Version& v : versions) {
				GetRuntimeConfig().versions[s.name].push_back(v);
			}
		}

		for(const auto &[server, versions] : GetRuntimeConfig().versions) {
			std::cout << "Fetched From " << server << ":" << std::endl;
			for(const auto& ver : versions) {
				std::cout << "\t[" << GREEN << server << "/" << ver.tag << RESET << "] : " << ver.url << "\n";
			}
		}
	}
}