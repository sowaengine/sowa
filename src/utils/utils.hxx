#ifndef SW_UTILS_HXX
#define SW_UTILS_HXX
#pragma once

#include <chrono>
#include <iostream>
#include <string>
#include <vector>

#include "fmt/core.h"

#define RESET "\033[0m"
#define BLACK "\033[30m"
#define RED "\033[31m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define BLUE "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN "\033[36m"
#define WHITE "\033[37m"
#define BOLDBLACK "\033[1m\033[30m"
#define BOLDRED "\033[1m\033[31m"
#define BOLDGREEN "\033[1m\033[32m"
#define BOLDYELLOW "\033[1m\033[33m"
#define BOLDBLUE "\033[1m\033[34m"
#define BOLDMAGENTA "\033[1m\033[35m"
#define BOLDCYAN "\033[1m\033[36m"
#define BOLDWHITE "\033[1m\033[37m"

namespace Utils {

//-------------------------   Time  -------------------------//

std::string GetTime(const std::string &format = "%Y-%m-%d");

//-------------------------   String  -------------------------//

std::vector<std::string> Split(std::string str, std::string delimiter);

template <typename... Args>
std::string Format(const std::string &format, Args... args) {
	return fmt::format(format, args...);
}

//-------------------------   Logging  -------------------------//

template <typename... Args>
void Print(const std::string levelText, const std::string &format, Args... args) {
	std::string time = GetTime("%Y-%m-%d %H:%M:%S");
	std::cout << Format("[{}] [{}] ", levelText, time) << Format(format, args...) << std::endl;
}

template <typename... Args>
void Error(const std::string &format, Args... args) {
	std::cout << RED;
	Print("ERR", format, args...);
	std::cout << RESET;
}

template <typename... Args>
void Warn(const std::string &format, Args... args) {
	std::cout << YELLOW;
	Print("WARN", format, args...);
	std::cout << RESET;
}

template <typename... Args>
void Info(const std::string &format, Args... args) {
	std::cout << CYAN;
	Print("INFO", format, args...);
	std::cout << RESET;
}

template <typename... Args>
void Log(const std::string &format, Args... args) {
	std::cout << GREEN;
	Print("LOG", format, args...);
	std::cout << RESET;
}

//-------------------------   Random  -------------------------//

/// @brief Randomizes random number generation
void Randomize();

/// @brief Wraps rand() function
int Rand();

/// @brief Returns an int between min and max.
/// max >= min, swaps min and max
/// max == min, returns max
int RandRange(int min, int max);

/// @brief Returns a random float between 0 and 1
float RandFloat();

/// @brief Returns a float between min and max.
/// max >= min, swaps min and max
/// max == min, returns min
float RandRangeFloat(float min, float max);

//-------------------------   Timer  -------------------------//

enum class TimeUnit {
	Milliseconds = 0,
	Seconds,
	Microseconds
};

class ScopeTimer {
	using TimePoint = std::chrono::time_point<std::chrono::high_resolution_clock>;

  public:
	ScopeTimer(const std::string &name, TimeUnit unit = TimeUnit::Milliseconds)
		: m_name(name),
		  m_start(std::chrono::high_resolution_clock::now()),
		  m_unit(unit) {
	}

	~ScopeTimer() {
		TimePoint now = std::chrono::high_resolution_clock::now();

		float dt = 0.f;
		if (m_unit == TimeUnit::Seconds) {
			dt = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_start).count() * 0.001f;
		} else if (m_unit == TimeUnit::Milliseconds) {
			dt = std::chrono::duration_cast<std::chrono::microseconds>(now - m_start).count() * 0.001f;
		} else if (m_unit == TimeUnit::Microseconds) {
			dt = std::chrono::duration_cast<std::chrono::nanoseconds>(now - m_start).count() * 0.001f;
		}
		Log("Timer {}: {}{}", m_name, dt, get_unit());
	};

  private:
	std::string m_name;
	TimePoint m_start;
	TimeUnit m_unit = TimeUnit::Milliseconds;

	const char *get_unit() {
		if (m_unit == TimeUnit::Milliseconds)
			return "ms";
		if (m_unit == TimeUnit::Seconds)
			return "s";
		if (m_unit == TimeUnit::Microseconds)
			return "μs";

		return "";
	}
};

} // namespace Utils

#endif // SW_UTILS_HXX