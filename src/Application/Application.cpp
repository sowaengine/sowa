#include "Application.hpp"
#include "Debug.hpp"
#include "Resource/Project/Project.hpp"

#include "Resource/Texture/Texture.hpp"
#include "Global.hpp"

#include "Scene/SceneTree.hpp"
#include "Scene/Node.hpp"
#include "Scene/Components.hpp"
#include "Scene/SystemDriver.hpp"


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
   SystemDriver systemDriver(Global::SceneTree->GetRegistry());

   std::shared_ptr<Texture> tex = Global::ResourceManager->LoadTexture("ship.png");
   std::shared_ptr<Texture> tex2 = Global::ResourceManager->LoadTexture("ship.png");
   Node* node1 = Global::SceneTree->NewNode("My Node1");
   Node* node2 = Global::SceneTree->NewNode("My Node2");
   
   auto& tc = node1->addComponent<Component::Transform2D>();
   auto& sprite = node1->addComponent<Component::Sprite2D>();

   tc.position = {0.0f, 0.0f};
   tc.scale = {1.f, 1.f}; 
   
   sprite.texture = tex;

   node2->addComponent<Component::Transform2D>();
   node2->addComponent<Component::Camera2D>();
   node2->getComponent<Component::Camera2D>().current = true;
   node2->addComponent<Component::Sprite2D>();
   node2->getComponent<Component::Sprite2D>().texture = tex2;

   node1->AddChild(node2);

   Node* node = Global::SceneTree->GetRoot()->GetNode("/My Node1/My Node2");


   glClearColor(0.15, 0.15, 0.15, 1);
   glEnable(GL_DEPTH_TEST);
   do
   {
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      tc.rotation += glm::radians(0.5f);
      systemDriver.UpdateAll();

      static int ticks = 0;
      ticks++;
      node2->getComponent<Component::Transform2D>().position.x += glm::sin(glm::radians(float(ticks))) * 3;


      glfwSwapBuffers(m_Window.GetPtr());
      glfwPollEvents();

   } while (
      glfwGetKey(m_Window.GetPtr(), GLFW_KEY_ESCAPE ) != GLFW_PRESS &&
      glfwWindowShouldClose(m_Window.GetPtr()) == 0 );

   Quit(0);
}

void cl_Application::Quit(int code /*= 0*/)
{
   exit(code);
}

   
} // namespace Ease
