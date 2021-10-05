#include "Window.hpp"
#include "Debug.hpp"
#include "Ease.hpp"

#include <iostream>

namespace Ease
{

Window::Window(int width, int height, const std::string& title)
   : width{width} ,height{height}, title{title}
{
   Debug::test_and_throw(!glfwInit(), "Cannot initialize GLFW");

   glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
   glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

   windowPointer = glfwCreateWindow(this->width,
                                    this->height,
                                    this->title.c_str(),
                                    nullptr,
                                    nullptr);
}

Window::~Window()
{
   glfwDestroyWindow(windowPointer);
   glfwTerminate();
}

} // namespace Ease
