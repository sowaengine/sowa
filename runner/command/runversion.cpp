#include "../commands.hpp"
#include "../filesystem.hpp"
#include "../utils.hpp"

void RunVersion(std::string version, std::filesystem::path appPath, std::vector<std::string> args) {
	std::string path = "";
	BranchVersionPair ver = ResolveFullVersion(version);

	if(ver.version == "latest") {
		std::string latest = GetLatestVersion((appPath / "bin" / ver.branch).string());
		if (latest == "") {
			std::cout << "Application not found" << std::endl;
			return;
		}
		path = ((appPath / "bin" / ver.branch) / latest).string();
	} else {
		std::string filename = GetVersionInPath((appPath / "bin" / ver.branch).string(), ver.version);
		if(filename == "") {
			std::cout << "Application not found" << std::endl;
			return;
		}
		path = ((appPath / "bin" / ver.branch) / filename).string();
	}

	if (path == "") {
		CommandHelp();
		exit(0);
	}

	FILE *handle = OpenExecutable(path, args);
	if (NULL == handle) {
		std::cerr << "Failed to open Application" << std::endl;
	}

	char data[512];
	while (fgets(data, 512, handle) != NULL) {
		std::cout << data;
	}

	pclose(handle);
}