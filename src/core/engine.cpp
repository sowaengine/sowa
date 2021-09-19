#include "engine.hpp"

#include <iostream>

#include <GLFW/glfw3.h>

#include "global.hpp"
#include "scene/node/node.hpp"

#include "scene/components/components.hpp"
#include "entt/entt.hpp"

#include "renderer/batch2d/batch2d.hpp"


Ease::Window* windowRef;

float rot = 0.f;

namespace Ease
{
   Engine::Engine()
      : m_Running(false)
   {
      
      /* Using GL 3.3 Core */
      glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
      glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

      glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

      glClearColor(0.2f, 0.3f, 0.3f, 1.0f);

      glEnable(GL_DEPTH_TEST);
      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);


      auto& monitor = *glfwGetVideoMode( glfwGetPrimaryMonitor() );

      glfwWindowHint(GLFW_RED_BITS, monitor.redBits);
      glfwWindowHint(GLFW_BLUE_BITS, monitor.blueBits);
      glfwWindowHint(GLFW_GREEN_BITS, monitor.greenBits);
      glfwWindowHint(GLFW_REFRESH_RATE, monitor.refreshRate);
   }
   unsigned int Engine::Update()
   {
      glfwPollEvents();

      // Render
      // Clear the colorbuffer
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


      if( m_Running )
      {
         /**
          * Process all components
          **/
         // Sprite2D - Transform2D
         {
            Renderer::setIndexCount(0);
            std::array<Renderer::Vertex, Renderer::maxVertexCount> vertices;
            Renderer::Vertex* buffer = vertices.data();

            Renderer::setVerticesPointer(&vertices);

            auto view = sceneRegistry.view<Comp::Sprite2D, Comp::Transform2D>();
            for(auto entity : view) {
               auto &sprite2d = view.get<Comp::Sprite2D>(entity);
               auto &transform2d = view.get<Comp::Transform2D>(entity);

               buffer = Renderer::CreateQuad(
                  buffer,
                  glm::vec3(1280/2, 720/2, 0),
                  glm::vec4(0.2f, 0.2f, 1.f, 1.f),
                  glm::vec2(256, 256),
                  rot+=0.2f,
                  1 // texid
               );


               // sprite2d.Draw();
            }
            Renderer::EndBatch();
         }
      }
      


      // Swap the screen buffers
      glfwSwapBuffers( windowRef->Get() );


      if(glfwWindowShouldClose( windowRef->Get()))
         return 0;

      return 1;
   }

   void Engine::SetRunning(bool will_run)
   {
      m_Running = will_run;
      if(m_Running)
      {
         // Start game
      } else {
         // End Game
      }
   }
}