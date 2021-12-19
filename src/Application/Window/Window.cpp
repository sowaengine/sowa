#include "glad/glad.h"
#include <GLFW/glfw3.h>

#include "Window.hpp"
#include <string>

#include "Debug.hpp"
#include <iostream>
#include "Event/Input/Keyboard.hpp"

void error_callback( int error, const char *msg ) {
    std::string s;
    s = " [" + std::to_string(error) + "] " + msg + '\n';
    std::cerr << s << std::endl;
}

namespace Ease
{

   void WindowResizeCallback(GLFWwindow* window, int width, int height);

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

      int w, h;
      glfwGetWindowSize(m_WindowPointer, &w, &h);
      m_WindowWidth = w;
      m_WindowHeight = h;

      // glfwSetWindowOpacity(m_WindowPointer, 0.5f);
      glfwSetWindowUserPointer(m_WindowPointer, this);
      glfwSetFramebufferSizeCallback(m_WindowPointer, WindowResizeCallback);

      glfwSetKeyCallback(m_WindowPointer, Input::KeyCallback);
      glfwSetInputMode(m_WindowPointer, GLFW_STICKY_KEYS, GL_TRUE);
   }



   /**
    * @brief class That updates Window class's m_Width and m_Height members
    * Doesn't change real window resolution
    */
   class WindowResizeUpdater
   {
      public:
         static void Update(Window* window, int width, int height)
         {
            window->m_WindowWidth = width;
            window->m_WindowHeight = height;

            LOG("New size: %d:%d", width, height)
         }
   };

   void WindowResizeCallback(GLFWwindow* window, int width, int height)
   {
      Window* m_Window = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
      WindowResizeUpdater::Update(m_Window, width, height);
   }   
} // namespace Ease
