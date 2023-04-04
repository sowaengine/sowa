#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include "argparser/impl.hpp"

#include "commands.hpp"
#include "filesystem.hpp"
#include "runner.hpp"
#include "utils.hpp"
#include "config.hpp"

std::filesystem::path appPath;

int main(int argc, char const *argv[]) {
	auto args = ArgParser(argc - 1, argv + 1);
	appPath = GetExecutableDir();
	LoadConfig(appPath);
	auto& cfg = GetConfig();
	if(cfg.checkUpdates) {
		if(cfg.lastUpdateCheck < time(nullptr) - cfg.checkFrequency) {
			std::cout << "Checking for updates..." << std::endl;


			cfg.lastUpdateCheck = time(nullptr);
			SaveConfig();
		}
	}

	auto passed = args.GetOption("pass", ArgOptionValue());
	std::vector<std::string> passedArgs;
	if(passed.value != "") {
		passedArgs = Split(passed.value, " ");
	}
	
	if (args.HasFlag("help") || args.HasFlag("h")) {
		CommandHelp();
		exit(0);
	}
	if (args.HasFlag("version") || args.HasFlag("v")) {
		CommandVersion();
		exit(0);
	}
	if (args.HasFlag("list") || args.HasFlag("l")) {
		CommandList(appPath, args.HasFlag("local"));
		exit(0);
	}
	if (args.HasFlag("update") || args.HasFlag("u")) {
		CommandUpdate();
		exit(0);
	}

	if(args.HasFlag("install") && args.GetParameters().size() == 1) {
		InstallVersion(args.GetParameters()[0], appPath);
		exit(0);
	}

	auto autoUpdateOpt = args.GetOption("auto-update");
	if(autoUpdateOpt.value != "") {
		GetConfig().checkUpdates = autoUpdateOpt.As<bool>();
		SaveConfig();
		exit(0);
	}

	if(args.HasFlag("run") && args.GetParameters().size() == 1) {
		RunVersion(args.GetParameters()[0], appPath, passedArgs);
		exit(0);
	}

	

    if(args.HasFlag("activate") && args.GetParameters().size() == 1) {
        ActivateVersion(args.GetParameters()[0]);
        exit(0);
    }
	
	if(args.GetOption("pass").value == "" && argc != 1) {
		CommandHelp();
		exit(0);
	}
	RunVersion(GetConfig().activeVersion, appPath, passedArgs);
	return 0;
}


