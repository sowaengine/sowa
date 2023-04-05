#include <iostream>
#include "../filesystem.hpp"
#include "../net.hpp"
#include "../config.hpp"
#include "../utils.hpp"
#include "../console.hpp"
#include "../runner.hpp"

void CommandList(std::filesystem::path appPath, bool isLocal) {
	if(isLocal) {
		auto versions = GetLocalVersions((appPath / "bin").string());

		std::cout << "Available sowa versions:\n";
		for (size_t i = 0; i < versions.size(); i++) {
			std::cout << "\t" << i << ". " << versions[i].path().parent_path().filename().string() << "/" << versions[i].path().filename().string() << "\n";
		}
		std::cout << "\n";
		std::cout << APP_NAME " --run {version}\n";
		std::cout << APP_NAME " --activate {version}\n";
		std::cout << std::endl;
	} else {
		std::cout << "Available sowa versions:\n";
		GetVersionsFromServer(GetConfig().servers);

		for(const auto &[server, versions] : GetRuntimeConfig().versions) {
			if(versions.size() > 0) {
				std::cout << "Fetched From " << server << ":" << std::endl;
				for(const auto& ver : versions) {
					std::string path = GetVersionInPath((appPath / "bin" / server), ver.tag);
					std::cout << "\t[" << GREEN << server << "/" << ver.tag << RESET << "] : ";
					if(path != "") {
						std::cout << MAGENTA << "[Installed] " << RESET;
					}
					
					std::cout << ver.url << "\n";
				}
			}
		}
	}
}