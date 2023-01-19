#ifndef _E_STRING_HPP__
#define _E_STRING_HPP__
#pragma once

#include "fmt/core.h"
#include "stlpch.hpp"

namespace Sowa {
std::vector<std::string> Split(const std::string &text, const std::string &delimiter);

template <typename... Args>
std::string Format(const std::string &format, Args... args) {
	return fmt::format(format, args...);
}
} // namespace Sowa

#endif // _E_STRING_HPP__