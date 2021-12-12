#include "Application.hpp"
#include "Debug.hpp"
#include "Resource/Project/Project.hpp"

#include "Resource/Texture/Texture.hpp"
#include "Global.hpp"

#include "Scene/SceneTree.hpp"
#include "Scene/Node.hpp"


namespace Ease
{

//static
void cl_Application::Init()
{
   static cl_Application app{};
   Global::Application = &app;
}

void cl_Application::InitApp()
{
   const ProjectSettings& projectSettings = Global::Project->GetProjectSettings();

   m_Window.Create(projectSettings.WindowWidth, projectSettings.WindowHeight, projectSettings.Title);
}


void cl_Application::Run()
{
   std::shared_ptr<Texture> tex = Global::ResourceManager->LoadTexture("ship.png");
   Node* node1 = Global::SceneTree->NewNode("My Node1");
   Node* node2 = Global::SceneTree->NewNode("My Node2");
   
   node1->AddChild(node2);

   Node* node = Global::SceneTree->GetRoot()->GetNode("/My Node1/My Node2");

   LOG("Node2 Path: %s", node2->GetPath().c_str())
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
