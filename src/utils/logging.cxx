#include "utils.hxx"

#include <iostream>

#include "core/app.hxx"

void Utils::WriteStdout(const std::string &line) {
	std::cout << line << std::endl;
	App::GetInstance().Log(line);
}