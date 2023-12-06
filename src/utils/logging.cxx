#include "utils.hxx"

#include <iostream>

#include "core/app.hxx"

void Utils::WriteStdout(const std::string &line) {
	std::cout << line;
	App::get().Log(line);
}