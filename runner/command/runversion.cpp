#include "../commands.hpp"
#include "../filesystem.hpp"

void RunVersion(std::string version, std::filesystem::path appPath, std::vector<std::string> args) {
	std::string path = "";
	if (version == "latest" || version == "") {
		std::string latest = GetLatestVersion((appPath / "bin/main").string());
		if (latest == "") {
			std::cout << "Application not found" << std::endl;
			return;
		}
		path = ((appPath / "bin/main") / latest).string();
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