#ifndef _E_STRING_HPP__
#define _E_STRING_HPP__
#pragma once

#include "fmt/core.h"
#include <string>
#include <vector>

namespace Ease {
std::vector<std::string> Split(const std::string &text, const std::string &delimiter);

template <typename... Args>
std::string Format(const std::string &format, Args... args) {
	return fmt::format(format, args...);
}
} // namespace Ease

#endif // _E_STRING_HPP__