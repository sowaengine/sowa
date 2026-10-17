#include "core/string.hpp"

#include <cctype>

namespace sowa::String {

std::vector<std::string> Split(std::string_view str, std::string_view delimiter) {
	std::vector<std::string> tokens;
	if (delimiter.empty()) {
		tokens.emplace_back(str);
		return tokens;
	}

	std::size_t start = 0;
	while (start <= str.size()) {
		std::size_t pos = str.find(delimiter, start);
		if (pos == std::string_view::npos) {
			tokens.emplace_back(str.substr(start));
			break;
		}
		tokens.emplace_back(str.substr(start, pos - start));
		start = pos + delimiter.size();
	}
	return tokens;
}

static bool isWhitespace(unsigned char c) {
	return std::isspace(c) != 0;
}

std::string TrimLeft(std::string_view str) {
	std::size_t i = 0;
	while (i < str.size() && isWhitespace(static_cast<unsigned char>(str[i]))) {
		++i;
	}
	return std::string(str.substr(i));
}

std::string TrimRight(std::string_view str) {
	std::size_t i = str.size();
	while (i > 0 && isWhitespace(static_cast<unsigned char>(str[i - 1]))) {
		--i;
	}
	return std::string(str.substr(0, i));
}

std::string Trim(std::string_view str) {
	std::size_t begin = 0;
	while (begin < str.size() && isWhitespace(static_cast<unsigned char>(str[begin]))) {
		++begin;
	}

	std::size_t end = str.size();
	while (end > begin && isWhitespace(static_cast<unsigned char>(str[end - 1]))) {
		--end;
	}

	return std::string(str.substr(begin, end - begin));
}

std::string ToLower(std::string_view str) {
	std::string lowered(str);
	for (char &c : lowered) {
		if (c >= 'A' && c <= 'Z') {
			c = static_cast<char>(c - 'A' + 'a');
		}
	}
	return lowered;
}

bool StartsWith(std::string_view str, std::string_view prefix) {
	return str.size() >= prefix.size() && str.substr(0, prefix.size()) == prefix;
}

bool EndsWith(std::string_view str, std::string_view suffix) {
	return str.size() >= suffix.size() && str.substr(str.size() - suffix.size()) == suffix;
}

std::string Join(const std::vector<std::string> &parts, std::string_view delimiter) {
	if (parts.empty()) {
		return {};
	}

	std::size_t total = delimiter.size() * (parts.size() - 1);
	for (const auto &part : parts) {
		total += part.size();
	}

	std::string result;
	result.reserve(total);
	result += parts[0];
	for (std::size_t i = 1; i < parts.size(); ++i) {
		result.append(delimiter);
		result += parts[i];
	}
	return result;
}

} // namespace sowa::String
