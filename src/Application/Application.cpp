#include "Application.hpp"
#include "Debug.hpp"
#include "Resource/Project/Project.hpp"

#include "Resource/Texture/Texture.hpp"
#include "Global.hpp"

#include "Scene/SceneTree.hpp"
#include "Scene/Node.hpp"
#include "Scene/Components.hpp"
#include "Scene/SystemDriver.hpp"

#include "Event/Input/Input.hpp"
#include "Event/Timer/Timer.hpp"

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
   SystemDriver systemDriver(Global::SceneTree->GetRegistry(), &m_Window);
   Timer m_Timer;


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
   node2->getComponent<Component::Sprite2D>().texture = tex;

   node1->AddChild(node2);

   for(int y = 0; y < 32; y++)
   {
      for(int x = 0; x < 32; x++)
      {
         Node* node = Global::SceneTree->NewNode( std::string( std::to_string(x) + "-" + std::to_string(y))  );
         node->addComponent<Component::Transform2D>();
         node->getComponent<Component::Transform2D>().position = {x*64, y*64};
         node->getComponent<Component::Transform2D>().scale = {0.2, 0.2};
         node->getComponent<Component::Transform2D>().rotation = x*y;
         node->addComponent<Component::Sprite2D>();
         node->getComponent<Component::Sprite2D>().texture = tex;
      }
   }

   glClearColor(0.15, 0.15, 0.15, 1);
   glEnable(GL_DEPTH_TEST);
   do
   {
      m_Timer.Update();
      
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      systemDriver.UpdateAll();
      
      tc.rotation += glm::radians(0.5f);
      if(Input::IsKeyPressed(KeyCode::KEY_D))
         node2->getComponent<Component::Transform2D>().position.x += 2.f;
      if(Input::IsKeyPressed(KeyCode::KEY_A))
         node2->getComponent<Component::Transform2D>().position.x -= 2.f;
      if(Input::IsKeyPressed(KeyCode::KEY_W))
         node2->getComponent<Component::Transform2D>().position.y += 2.f;
      if(Input::IsKeyPressed(KeyCode::KEY_S))
         node2->getComponent<Component::Transform2D>().position.y -= 2.f;

      if(Input::IsKeyPressed(KeyCode::KEY_SPACE))
         tex2 = nullptr;

      glfwSwapBuffers(m_Window.GetPtr());
      glfwPollEvents();

   } while (
      !Input::IsKeyPressed(KeyCode::KEY_ESCAPE) &&
      glfwWindowShouldClose(m_Window.GetPtr()) == 0 );

   Quit(0);
}

void cl_Application::Quit(int code /*= 0*/)
{
   exit(code);
}

   
} // namespace Ease
