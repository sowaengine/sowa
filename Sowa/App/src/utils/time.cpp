#include "time.hpp"

#include "stlpch.hpp"

namespace Sowa {
namespace Time {
std::string GetTime(const std::string &format /*= "%Y-%m-%d"*/) {
	std::stringstream date;
	std::time_t t = std::time(nullptr);
	std::tm tp = *std::localtime(&t);
	date << std::put_time(&tp, format.c_str());
	return date.str();
}
} // namespace Time

} // namespace Sowa
