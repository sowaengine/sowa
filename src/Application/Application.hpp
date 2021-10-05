#pragma once

#include "Window/Window.hpp"


namespace Ease
{

class Application
{
   /* objects that initialize at startup */
   public:
      Window window{800, 600, "Ease Engine Vulkan"};

   public:
      /**
       * Entry point and main loop
       **/
      void Run();
};

} // namepace Ease