/**
 * @file Timer.hpp
 * @author Lexographics
 * @brief 
 * @version 0.1
 * @date 2021-12-19
 */
#pragma once

#include "Debug.hpp"
#include <chrono>

namespace Ease
{

class Timer
{
   public:
      Timer();

      void Update();

      double deltaTime = 0.0;

      inline uint64_t Ticks() { return m_FramePast; }
      inline uint32_t FPS() { return 1.f / deltaTime; }

   private:
      uint64_t m_FramePast = 0;
      std::chrono::_V2::steady_clock::time_point m_StartTime;

};

}