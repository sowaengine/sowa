#include "Application.h"
//#include "Debug.hpp"
#include <iostream>
#include "Ease.hpp"

#include "raylib.h"


#include "Core/Renderer.h"

#include "Resource/ResourceManager.h"
#include "Resource/NativeModule/NativeModule.h"

#include "Window.h"
#include "ProjectSettings.h"

#include "ECS/Entity/Entity.h"
#include "ECS/Scene/Scene.h"
#include "ECS/Components/Components.hpp"
#include "ECS/Systems/Systems.h"

#include "imgui-docking/imgui.h"
#include "rlImGui/rlImGui.h"

#define EASE_EDITOR

namespace Ease
{
   Application::Application()
      : m_pCurrentScene(&m_GameScene)
   {
   }

   Application::~Application()
   {
   }
   

   void Application::Run()
   {
      SetTraceLogLevel(LOG_ERROR);
      
      //INFO("Ease Engine v{}.{}.{}", EASE_VERSION_MAJOR, EASE_VERSION_MINOR, EASE_VERSION_PATCH);

      ProjectSettings& projectSettings = ProjectSettings::get_singleton();

      projectSettings.LoadProject("./TestGame");

      m_Window.CreateWindow(
         projectSettings._window.WindowWidth,
         projectSettings._window.WindowHeight,
         projectSettings._application.Name
      );
      m_Window.SetVideoSize(
         projectSettings._window.VideoWidth,
         projectSettings._window.VideoHeight
      );

      LoadModule("Ease", "Core", 1);
      #ifdef EASE_EDITOR
         LoadModule("Ease", "Editor", 1);
      #endif

      
      /*Entity ent1 = m_pCurrentScene->Create("Entity1");
      auto& transform2d = ent1.AddComponent<Component::Transform2D>();
      transform2d.Position() = {200.f, -200.f};
      transform2d.Scale() = {10.f, 10.f};

      std::vector<Ease::Entity> entities;
      {
         Ease::Entity text = m_pCurrentScene->Create("Text");
         auto& text_tc = text.AddComponent<Component::Transform2D>();
         text_tc.Position() = {600.f, -300.f};

         auto& text_font = text.AddComponent<Component::TextRenderer2D>();
         text_font.Text() = "EASE ENGINE";
         text_font.Color() = {25.f, 4.f, 202.1f, 255.f};
         
         entities.push_back(text);
      }
      {
         Ease::Entity text = m_pCurrentScene->Create("Text_Ease");
         auto& text_tc = text.AddComponent<Component::Transform2D>();
         text_tc.Position() = {1200.f, -300.f};

         auto& text_font = text.AddComponent<Component::TextRenderer2D>();
         text_font.Text() = "Ease Engine";
         text_font.Color() = {4.f, 202.f, 20.21f, 255.f};
         
         entities.push_back(text);
      }
      {
         Ease::Entity icon = m_pCurrentScene->Create("icon");
         auto& tc = icon.AddComponent<Component::Transform2D>();
         tc.Position() = {800, -650};
         tc.Scale() = {0.25f, 0.25f};

         auto& sprite = icon.AddComponent<Component::SpriteRenderer2D>();
         sprite.TextureID() = iconTex->GetResourceID();
      }

      
      Ease::Animation anim;
      anim.TextureID = tex->GetResourceID();
      anim.hFrames = 23;
      anim.vFrames = 4;
      anim.SelectedRow = 1;
      anim.frameCount = 18;
      anim.startFrame = 0;
      
      auto& animSprite = ent1.AddComponent<Component::AnimatedSprite2D>();
      animSprite.SetAnimation("anim", anim);
      animSprite.SelectedAnimation() = "anim";
      animSprite.FPS() = 4;*/

      IMGUI_CHECKVERSION();
      SetupRLImGui(true);
      ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;

      InitModules();
      m_pCurrentScene->LoadFromFile(projectSettings._application.MainScene.c_str());

      #ifndef EASE_EDITOR
         StartGame();
      #endif
      while (!m_Window.ShouldClose())
      {
         m_Window.Begin();
         
         Ease::Systems::ProcessAll(m_pCurrentScene, m_AppRunning ? SystemsFlags::Update_WhenRunning : SystemsFlags::Update_Always);

         if(m_AppRunning)
            UpdateGame();

         UpdateModules();


         m_Window.End();
      }
   }

   void Application::Log(const std::string& message)
   {
      std::cout << "[APP] " << message << std::endl;

      static std::shared_ptr<NativeModule> editor = GetModule("Ease.Editor");
      if(editor != nullptr)
      {
         editor->GetModule()->userValues["PrintMsg"].str_value = message;
         editor->GetModule()->userFuncs["Print"]();
      }
   }
   
   void Application::StartGame()
   {
      if(m_AppRunning) return;
      m_AppRunning = true;

      m_pCurrentScene->StartScene();
   }


   void Application::UpdateGame()
   {
      
   }

   void Application::StopGame()
   {
      if(!m_AppRunning) return;
      m_AppRunning = false;

      
   }
   


   // Initialize loaded native modules
   void Application::InitModules()
   {
      for(auto& pair : m_Modules)
         pair.second->CallStart();
   }

   void Application::UpdateModules()
   {
      for(auto& pair : m_Modules)
         pair.second->CallUpdate();
   }
   void Application::Modules_OnImGuiRender()
   {
      for(auto& pair : m_Modules)
         pair.second->CallOnImGuiRender();
   }

   Application::ModuleLoadResult Application::LoadModule(const std::string& author, const std::string& moduleName, int minimumVersion)
   {
      static ResourceManager<NativeModule>& moduleLoader = ResourceManager<NativeModule>::GetLoader();

      std::string modulePath = "modules/";
      modulePath += author;
      modulePath += ".";
      modulePath += moduleName;

      std::shared_ptr<NativeModule> myModule = moduleLoader.LoadResource(modulePath.c_str());
      
      if(author != myModule->m_pModule->metadata.authorName || moduleName != myModule->m_pModule->metadata.moduleName)
         return ModuleLoadResult::UNDEFINED_MODULE;
      if(myModule->m_pModule->metadata.version < minimumVersion)
         return ModuleLoadResult::OUTDATED_VERSION;

      
      AddModule(std::string(author + "." + moduleName), myModule);
      return ModuleLoadResult::SUCCESS;
   }

   void Application::AddModule(const std::string& name, std::shared_ptr<NativeModule> _module)
   {
      m_Modules[name] = _module;
   }
   std::shared_ptr<NativeModule> Application::GetModule(const std::string& name)
   {
      if(m_Modules.count(name) == 1)
         return m_Modules[name];
      else
         return nullptr;
   }
   
   
} // namespace Ease