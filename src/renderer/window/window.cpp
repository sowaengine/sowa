#include "window.hpp"
#include <iostream>

#include "errorCodes.hpp"

#include "glad/glad.h"
#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

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


      // set window icon
      GLFWimage images[1]; 
      images[0].pixels = stbi_load("resources/Mechanic/idle_1.png", &images[0].width, &images[0].height, 0, 4); //rgba channels 
      glfwSetWindowIcon(windowPointer, 1, images); 
      stbi_image_free(images[0].pixels);
   }


}
