#include "engine.hpp"

#include <GLFW/glfw3.h>

#include "global.hpp"


Ease::Window* windowRef;


namespace Ease
{
   unsigned int Engine::Update()
   {
      glfwPollEvents();

      // Render
      // Clear the colorbuffer
      glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
      glClear(GL_COLOR_BUFFER_BIT);

      // Swap the screen buffers
      glfwSwapBuffers( windowRef->Get() );


      if(glfwWindowShouldClose( windowRef->Get()))
         return 0;

      return 1;
   }

}