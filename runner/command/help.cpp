#include "../commands.hpp"

#include <iostream>
#include "../runner.hpp"

void CommandHelp() {
    std::cout << "Sowa runner v" RUNNER_VERSION "\n";
    std::cout << "Commands: \n";
    std::cout << "\trunner                      - runs active version\n";
    std::cout << "\trunner --help  [-h]         - display help information\n";
    std::cout << "\trunner --version  [-v]      - prints current version\n";
    std::cout << "\trunner --list [-l]          - prints installed versions\n";
    std::cout << "\trunner --update [-u]        - installs latest sowa version\n";
    std::cout << "\trunner --run {version}      - runs passed sowa version (see versions with runner --list)\n";
    std::cout << "\trunner --activate {version} - activates passed sowa version (see versions with runner --list)\n";

    std::cout << "\n\tvalue of--pass option will be passed to engine\n\trunner --pass='open ...'\n";
    std::cout << std::endl;
}