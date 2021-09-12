#ifndef __WINDOW_HPP__
#define __WINDOW_HPP__


#include "glad/glad.h"
#include <GLFW/glfw3.h>

namespace Ease
{
   class Window
   {
      private:
         GLFWwindow* windowPointer;
      public:
         Window(const char* title, int w=1280, int h=720);

         GLFWwindow* Get() { return windowPointer; }
   };
}


#endif // __WINDOW_HPP__