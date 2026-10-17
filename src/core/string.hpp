#pragma once

#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <fmt/format.h>

namespace sowa::String {

std::vector<std::string> Split(std::string_view str, std::string_view delimiter);

std::string Trim(std::string_view str);
std::string TrimLeft(std::string_view str);
std::string TrimRight(std::string_view str);

// ASCII only: locale-independent by design, so a file extension check
// behaves the same everywhere rather than following the host's locale.
std::string ToLower(std::string_view str);

bool StartsWith(std::string_view str, std::string_view prefix);
bool EndsWith(std::string_view str, std::string_view suffix);

std::string Join(const std::vector<std::string> &parts, std::string_view delimiter);

template <typename... Args> std::string Format(fmt::format_string<Args...> fmtStr, Args &&...args) {
	return fmt::format(fmtStr, std::forward<Args>(args)...);
}

} // namespace sowa::String
