#include "window.hpp"
#include <iostream>

#include "errorCodes.hpp"

#include "glad/glad.h"
#include <GLFW/glfw3.h>

namespace Ease
{
   Window::Window(const char* title, int w, int h)
   {
      if(!glfwInit())
         exit(ERROR_GLFW_INIT_FAILED);

      windowPointer = glfwCreateWindow(w, h, title, NULL, NULL);
      if (!windowPointer) {
         glfwTerminate();
         exit( ERROR_GLFW_WINDOW_CREATION );
      }

      glfwMakeContextCurrent(windowPointer);

      if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
         glfwTerminate();
         exit(ERROR_GLFW_WINDOW_CREATION);
      }
   }


}
