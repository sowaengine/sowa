#include "glad/glad.h"
#include <GLFW/glfw3.h>

#include "Window.hpp"
#include <string>

#include "Debug.hpp"
#include <iostream>

void error_callback( int error, const char *msg ) {
    std::string s;
    s = " [" + std::to_string(error) + "] " + msg + '\n';
    std::cerr << s << std::endl;
}

namespace Ease
{

void Window::Create(int width, int height, const std::string& title)
{
   glfwSetErrorCallback( error_callback );

   if( !glfwInit() )
   {
      throw std::runtime_error("Failed to initialize GLFW");
   }

   glfwWindowHint(GLFW_SAMPLES, 4);
   glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
   glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
   glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
   glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

   
   
   m_WindowPointer = glfwCreateWindow(width, height, title.c_str(), NULL, NULL);
   if(m_WindowPointer == nullptr)
   {
      throw std::runtime_error("Failed to create GLFW window");
   }

   glfwMakeContextCurrent(m_WindowPointer);
   gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
   glfwSwapInterval(1);


   glfwSetInputMode(m_WindowPointer, GLFW_STICKY_KEYS, GL_TRUE);
}
   
} // namespace Ease
