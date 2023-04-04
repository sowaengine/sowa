#include "../commands.hpp"

#include <iostream>
#include "../runner.hpp"

void CommandHelp() {
    std::cout << "Sowa runner v" RUNNER_VERSION "\n";
    std::cout << "Commands: \n";
    std::cout << "\t" APP_NAME "                             - runs active version\n";
    std::cout << "\t" APP_NAME " --help  [-h]                - display help information\n";
    std::cout << "\t" APP_NAME " --version  [-v]             - prints current version\n";
    std::cout << "\t" APP_NAME " --list [-l]                 - prints installed versions\n";
    std::cout << "\t" APP_NAME " --update [-u]               - installs latest sowa version\n";
    std::cout << "\t" APP_NAME " --auto-update={true|false}  - Toggles auto update with given value\n";
    std::cout << "\t" APP_NAME " --run {version}             - runs passed sowa version (see versions with runner --list)\n";
    std::cout << "\t" APP_NAME " --activate {version}        - activates passed sowa version (see versions with runner --list)\n";

    std::cout << "\n\tvalue of --pass option will be passed to engine\n\t" APP_NAME " --pass='--project=...'\n";
    std::cout << std::endl;
}