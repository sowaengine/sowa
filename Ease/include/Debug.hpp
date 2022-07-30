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

#include <stdlib.h>
#include <stdexcept>
#include <cassert>
#include <unistd.h>
#include <chrono>
#include "spdlog/spdlog.h"


//#define LOG(format, ...) fprintf (stderr, format "\n", __VA_ARGS__);
//#define LOG_ERROR(format, ...) fprintf (stderr, "ERROR: " format "\n" , __VA_ARGS__);

#define INFO(format, ...)  /* info doesn't save to log file */ \
                           spdlog::info(format, __VA_ARGS__);

#define WARNING(format, ...) spdlog::get("Ease")->warn(format, __VA_ARGS__); \
                             spdlog::warn(format, __VA_ARGS__);

#define ERROR(format, ...) spdlog::get("Ease")->error(format, __VA_ARGS__); \
                           spdlog::error(format, __VA_ARGS__);

namespace Debug
{
   class Timer
   {
      public:
         Timer(std::string name)
         : m_Name(name),
            m_Start(std::chrono::high_resolution_clock::now())
         {
         }

         ~Timer()
         {
            auto now = std::chrono::high_resolution_clock::now();
            auto deltaTime = std::chrono::duration_cast<std::chrono::microseconds>(now - m_Start).count();
            //LOG("Timer %s: %ldusec", m_Name.c_str(), deltaTime);
            INFO("Timer {}: {}ms", m_Name.c_str(), deltaTime*0.001);
         }
      private:
         std::string m_Name;
         std::chrono::time_point<std::chrono::high_resolution_clock> m_Start;
   };
}
#endif