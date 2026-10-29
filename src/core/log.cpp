#include "core/log.hpp"

#include <atomic>
#include <ctime>
#include <iostream>
#include <mutex>

namespace sowa::Log {
namespace {

constexpr std::size_t kDefaultMaxLines = 1000;
std::atomic<std::size_t> errorCount{0};
std::string lastErrorText;

std::mutex &mutex() {
	static std::mutex m;
	return m;
}

std::deque<Message> &lines() {
	static std::deque<Message> storage;
	return storage;
}

std::size_t &maxLines() {
	static std::size_t value = kDefaultMaxLines;
	return value;
}

std::string currentTimestamp() {
	std::time_t t = std::time(nullptr);
	std::tm tp{};
#if defined(_WIN32)
	localtime_s(&tp, &t);
#else
	localtime_r(&t, &tp);
#endif

	char buffer[32];
	if (std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &tp) == 0) {
		return "unknown-time";
	}
	return buffer;
}

const char *ansiColor(Severity severity) {
	switch (severity) {
	case Severity::Trace:
		return "\033[0;32m"; // green
	case Severity::Info:
		return "\033[0;36m"; // cyan
	case Severity::Warn:
		return "\033[0;33m"; // yellow
	case Severity::Error:
		return "\033[0;31m"; // red
	}
	return "\033[0m";
}

} // namespace

const char *SeverityToString(Severity severity) {
	switch (severity) {
	case Severity::Trace:
		return "TRACE";
	case Severity::Info:
		return "INFO";
	case Severity::Warn:
		return "WARN";
	case Severity::Error:
		return "ERROR";
	}
	return "UNKNOWN";
}

void Internal::Push(std::string_view text, Severity severity) {
	if (severity == Severity::Error) {
		++errorCount;
	}
	std::string line =
		fmt::format("[{}] [{}] {}", currentTimestamp(), SeverityToString(severity), text);

	{
		std::lock_guard<std::mutex> lock(mutex());
		if (severity == Severity::Error && !lines().empty() &&
			lines().back().severity == severity && lastErrorText == text) {
			const std::size_t repeats = ++lines().back().repetitions;
			// Report repeats at powers of two to limit per-frame error spam.
			if ((repeats & (repeats - 1)) == 0) {
				std::cout << ansiColor(severity) << line << " (repeated " << repeats
							<< " times)\033[0m\n";
			}
			return;
		}
		lastErrorText = severity == Severity::Error ? std::string(text) : std::string();
		std::cout << ansiColor(severity) << line << "\033[0m\n";

		lines().push_back(Message{std::move(line), severity});
		while (lines().size() > maxLines()) {
			lines().pop_front();
		}
	}
}

std::size_t ErrorCount() {
	return errorCount.load();
}

const std::deque<Message> &GetLines() {
	return lines();
}

void Clear() {
	std::lock_guard<std::mutex> lock(mutex());
	lines().clear();
}

std::size_t MaxLines() {
	std::lock_guard<std::mutex> lock(mutex());
	return maxLines();
}

void SetMaxLines(std::size_t value) {
	std::lock_guard<std::mutex> lock(mutex());
	maxLines() = value;
	while (lines().size() > maxLines()) {
		lines().pop_front();
	}
}

} // namespace sowa::Log
