#ifndef _E_TIME_HPP__
#define _E_TIME_HPP__
#pragma once

#include <string>

namespace sowa {
namespace Time {

std::string GetTime(const std::string &format = "%Y-%m-%d");

} // namespace Time
} // namespace sowa

#endif // _E_TIME_HPP__