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

#include "stlpch.hpp"

#include "fmt/core.h"

#include "utils/time.hpp"

namespace Debug {

enum class LogLevel : std::uint32_t {
	Log = 0,
	Info,
	Warn,
	Error
};

class Streams {
  public:
	Streams() = default;
	~Streams() = default;

	static Streams &GetInstance();

	std::vector<std::ostream *> &Get(uint32_t level) {
		return _Streams[level];
	}

	void Add(uint32_t level, std::ostream *stream) {
		_Streams[level].push_back(stream);
	}

	void SetLevelText(uint32_t level, const std::string &text) {
		_LevelTexts[level] = text;
	}
	const std::string &GetLevelText(uint32_t level) {
		return _LevelTexts[level];
	}

  private:
	std::unordered_map<uint32_t, std::vector<std::ostream *>> _Streams{};
	std::unordered_map<uint32_t, std::string> _LevelTexts{};
};

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
void Print(uint32_t level, const std::string &format, Args... args) {
	std::string time = Sowa::Time::GetTime("%Y-%m-%d %H:%M:%S");
	std::string msg = fmt::format("[{}] [{}]: {}\n", time, Streams::GetInstance().GetLevelText(level), fmt::format(format, args...));
	auto &streams = Streams::GetInstance().Get(level);
	for (size_t i = 0; i < streams.size(); i++) {
		std::ostream *stream = streams[i];
		*stream << msg << std::flush;
	}
}

// Only prints to console
template <typename... Args>
void Log(const std::string &format, Args... args) {
	Print((uint32_t)LogLevel::Log, format, args...);
}

template <typename... Args>
void Info(const std::string &format, Args... args) {
	Print((uint32_t)LogLevel::Info, format, args...);
}

//
template <typename... Args>
void Warn(const std::string &format, Args... args) {
	Print((uint32_t)LogLevel::Warn, format, args...);
}

//
template <typename... Args>
void Error(const std::string &format, Args... args) {
	Print((uint32_t)LogLevel::Error, format, args...);
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