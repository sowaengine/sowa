#include "Window.hpp"
#include <GLFW/glfw3.h>

#include "Ease.hpp"


namespace Ease
{

static void glfwWindowResizeCallback(GLFWwindow* windowPointer, int w, int h)
{
   Window* window = reinterpret_cast<Window*>(glfwGetWindowUserPointer(windowPointer));
   window->setWindowSize(w, h);
}


Window::Window()
{

}

void Window::create(int width, int height, const char* title)
{
   m_Width = width;
   m_Height = height;
   m_Title = title;

   m_WindowPointer = glfwCreateWindow(width, height, title, NULL, NULL);
   Debug::test_and_throw(m_WindowPointer, "GLFWWindow* is null");

   glfwMakeContextCurrent(m_WindowPointer);
   gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
   glfwSwapInterval(1);

   glfwSetWindowUserPointer(m_WindowPointer, this);
   glfwSetFramebufferSizeCallback(m_WindowPointer, glfwWindowResizeCallback);
   
}
   
} // namespace Ease
