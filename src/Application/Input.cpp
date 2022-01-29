#include "Input.hpp"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include "Window.hpp"

#include <iostream>

namespace Input
{
   bool IsKeyPressed(Key key)
   {
      static Window& window = Window::get_singleton();
      return glfwGetKey(window, (int)key) == GLFW_PRESS;
   }
} // namespace Input
