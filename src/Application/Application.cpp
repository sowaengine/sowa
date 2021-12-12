#include "Application.hpp"
#include "Debug.hpp"
#include "Resource/Project/Project.hpp"


namespace Ease
{

void cl_Application::InitApp()
{
   const ProjectSettings& projectSettings = Project.GetProjectSettings();

   m_Window.Create(projectSettings.WindowWidth, projectSettings.WindowHeight, projectSettings.Title);
}


void cl_Application::Run()
{
   do
   {
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      // Render Scene


      glfwSwapBuffers(m_Window.GetPtr());
      glfwPollEvents();

   } while (
      glfwGetKey(m_Window.GetPtr(), GLFW_KEY_ESCAPE ) != GLFW_PRESS &&
      glfwWindowShouldClose(m_Window.GetPtr()) == 0 );
}

void cl_Application::Quit(int code /*= 0*/)
{
   exit(code);
}

   
} // namespace Ease
