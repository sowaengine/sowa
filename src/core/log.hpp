#pragma once

#include <cstddef>
#include <deque>
#include <string>
#include <string_view>
#include <utility>

#include <fmt/format.h>

namespace sowa::Log {

enum class Severity {
	Trace,
	Info,
	Warn,
	Error,
};

struct Message {
	std::string text;
	Severity severity;
};

const char *SeverityToString(Severity severity);

const std::deque<Message> &GetLines();
void Clear();
std::size_t MaxLines();
void SetMaxLines(std::size_t maxLines);

namespace Internal {
void Push(std::string_view text, Severity severity);

template <typename... Args>
void Write(Severity severity, fmt::format_string<Args...> fmtStr, Args &&...args) {
	Push(fmt::format(fmtStr, std::forward<Args>(args)...), severity);
}
} // namespace Internal

template <typename... Args> void Trace(fmt::format_string<Args...> fmtStr, Args &&...args) {
	Internal::Write(Severity::Trace, fmtStr, std::forward<Args>(args)...);
}

template <typename... Args> void Info(fmt::format_string<Args...> fmtStr, Args &&...args) {
	Internal::Write(Severity::Info, fmtStr, std::forward<Args>(args)...);
}

template <typename... Args> void Warn(fmt::format_string<Args...> fmtStr, Args &&...args) {
	Internal::Write(Severity::Warn, fmtStr, std::forward<Args>(args)...);
}

template <typename... Args> void Error(fmt::format_string<Args...> fmtStr, Args &&...args) {
	Internal::Write(Severity::Error, fmtStr, std::forward<Args>(args)...);
}

} // namespace sowa::Log
