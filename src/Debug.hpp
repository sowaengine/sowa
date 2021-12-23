/**
 * @file Debug.hpp
 * @author Lexographics
 * @brief Debugging utilities
 * @version 0.1
 * @date 2021-12-12
 */
#pragma once
#include <stdlib.h>
#include <stdexcept>
#include <cassert>
#include <unistd.h>


#define LOG(format, ...) fprintf (stderr, format "\n", __VA_ARGS__);
#define LOG_ERROR(format, ...) fprintf (stderr, "ERROR: " format "\n" , __VA_ARGS__);

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
            LOG("Timer %s: %ldusec", m_Name.c_str(), deltaTime);
         }
      private:
         std::string m_Name;
         std::chrono::time_point<std::chrono::high_resolution_clock> m_Start;
   };
}