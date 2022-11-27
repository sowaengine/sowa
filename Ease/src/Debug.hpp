/**
 * @file Debug.hpp
 * @author Lexographics
 * @brief Debugging utilities
 * @version 0.2
 * @date 2021-12-12
 */
#ifndef DEBUG_HPP_
#define DEBUG_HPP_
#pragma once

#include <cassert>
#include <chrono>
#include <cmath>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stack>
#include <stdexcept>
#include <stdlib.h>
#include <unistd.h>

#include "fmt/core.h"

#include "Utils/Event.hpp"

namespace Debug {

Sowa::Event<void(std::string)> &OnPrint();

/**
 * @brief Internal print function used by Other logging functions
 * @see Debug::Log
 * @see Debug::Info
 * @see Debug::Error
 * @see Debug::Warn
 *
 * @tparam Args
 * @param levelText type of printing (Log, Error, Warn, Info etc.)
 * @param toFile
 * @param format
 * @param args
 */
template <typename... Args>
void Print(const std::string &levelText, bool toFile, const std::string &format, Args... args) {
	std::time_t t = std::time(nullptr);
	std::tm tp = *std::localtime(&t);
	std::stringstream time;
	time << std::put_time(&tp, "%Y-%m-%d %H:%M:%S");

	static std::ofstream logStream;
	static bool first = true;
	if (first) {
		first = false;

		std::stringstream date;
		date << std::put_time(&tp, "%Y-%m-%d");

		logStream.open(fmt::format("./sowa-{}.log", date.str()), std::ios_base::app);
	}

	std::string msg = fmt::format("[{}] [{}]: {}\n", time.str(), levelText, fmt::format(format, args...));

	std::cout << msg << std::flush;
	OnPrint().Invoke(msg);
	if (toFile)
		logStream << msg << std::flush;
}

// Only prints to console
template <typename... Args>
void Log(const std::string &format, Args... args) {
	Debug::Print("LOG", false, format, args...);
}

// Used for logging info that should be seen in log file (system spec etc.)
template <typename... Args>
void Info(const std::string &format, Args... args) {
	Debug::Print("INFO", true, format, args...);
}

//
template <typename... Args>
void Warn(const std::string &format, Args... args) {
	Debug::Print("WARN", true, format, args...);
}

//
template <typename... Args>
void Error(const std::string &format, Args... args) {
	Debug::Print("ERROR", true, format, args...);
}

class ScopeTimer {
  public:
	ScopeTimer(std::string name)
		: _Name(name),
		  _Start(std::chrono::high_resolution_clock::now()) {
	}

	~ScopeTimer() {
		auto now = std::chrono::high_resolution_clock::now();
		auto deltaTime = std::chrono::duration_cast<std::chrono::microseconds>(now - _Start).count();

		Debug::Log("Scope Timer '{}': {}s", _Name.c_str(), std::round(static_cast<float>(deltaTime) / 1000.f) / 1000.f);
	}

  private:
	std::string _Name;
	std::chrono::time_point<std::chrono::high_resolution_clock> _Start;
};

struct FuncEntry {
	std::string _FuncName;
	std::string _File;
	int _Line;

	FuncEntry();
	FuncEntry(const std::string &funcName, const std::string &file, int line);
	~FuncEntry();
};
typedef std::stack<FuncEntry> CallStack;

struct FuncEntryRegister {
	FuncEntryRegister() = delete;
	FuncEntryRegister(const std::string &funcName, const std::string &file, int line);
	~FuncEntryRegister();
};

void DumpStacktrace();

} // namespace Debug

#define SW_ENTRY() Debug::FuncEntryRegister __sw_entry(__FUNCTION__, __FILE__, __LINE__);

#endif