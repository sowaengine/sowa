#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <string>


namespace Ease
{
   
class Window
{
   private:
      GLFWwindow* windowPointer;
      int width;
      int height;
      std::string title;
      

   public:
      bool shouldClose() { return glfwWindowShouldClose(windowPointer); }
   public:
      Window(int width, int height, const std::string& title);
      ~Window();
};


} // namespace Ease
