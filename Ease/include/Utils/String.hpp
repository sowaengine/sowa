#ifndef _E_STRING_HPP__
#define _E_STRING_HPP__
#pragma once

#include <string>
#include <vector>

namespace Ease {
std::vector<std::string> Split(const std::string &text, const std::string &delimiter);
} // namespace Ease

#endif // _E_STRING_HPP__