#include "../commands.hpp"
#include "../console.hpp"
#include "../net.hpp"
#include "../utils.hpp"
#include <iostream>




void InstallVersion(std::string version, std::filesystem::path appPath) {
    BranchVersionPair ver = ResolveFullVersion(version);
	std::cout << "Installing version: " << CYAN << ver.Repr() << RESET << std::endl;
	GetVersionsFromServer(GetConfig().servers);
	for (auto v : GetRuntimeConfig().versions[ver.branch]) {
		if (version == v.tag) {
			InstallSowa(appPath, v.url, BranchVersionPair(ver.branch, v.tag));
		}
	}
}