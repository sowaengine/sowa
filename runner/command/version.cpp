#include "../commands.hpp"
#include "../runner.hpp"

#include <iostream>

void CommandVersion() {
	std::cout << "Sowa runner v" RUNNER_VERSION "\n";
	std::cout << "Sowa build: " SOWA_VERSION << std::endl;
}