#include "utils.hxx"

#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>

std::string Utils::GetTime(const std::string &format /*= "%Y-%m-%d"*/) {
	std::stringstream date;
	std::time_t t = std::time(nullptr);
	std::tm tp = *std::localtime(&t);
	date << std::put_time(&tp, format.c_str());
	return date.str();
}