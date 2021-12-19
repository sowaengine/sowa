#include "Timer.hpp"

#include <chrono>
#include <GLFW/glfw3.h>

namespace Ease
{

   Timer::Timer()
   {
      m_StartTime = std::chrono::steady_clock::now();
   }

   void Timer::Update()
   {
      static double lastUpdate = glfwGetTime();

      auto now = glfwGetTime();
      deltaTime = now - lastUpdate;
      lastUpdate = now;


      m_FramePast++;
   }
   
} // namespace Ease
