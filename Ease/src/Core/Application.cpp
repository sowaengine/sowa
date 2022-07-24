#include "Core/Application.hpp"
//#include "Debug.hpp"
#include <iostream>
#include "Ease.hpp"

#include "raylib.h"


#include "Core/Renderer.hpp"

#include "Resource/ResourceManager.hpp"
#include "Resource/NativeModule/NativeModule.hpp"

#include "Core/Window.hpp"
#include "Core/ProjectSettings.hpp"

#include "ECS/Entity/Entity.hpp"
#include "ECS/Scene/Scene.hpp"
#include "ECS/Components/Components.hpp"
#include "ECS/Systems/Systems.hpp"

#include "imgui-docking/imgui.h"
#include "rlImGui/rlImGui.h"

#include "Utils/File.hpp"
#include "Core/Input.hpp"
#include "Utils/Dialog.hpp"

namespace Ease
{
   Application::Application()
      : m_pCurrentScene(&m_GameScene)
   {
   }

   Application::~Application()
   {
   }
   

   void Application::Run(int argc, char const *argv[])
   {
      SetTraceLogLevel(LOG_WARNING);
      
      //INFO("Ease Engine v{}.{}.{}", EASE_VERSION_MAJOR, EASE_VERSION_MINOR, EASE_VERSION_PATCH);

      ProjectSettings& projectSettings = ProjectSettings::get_singleton();
      Ease::File::InsertFilepathEndpoint("abs", "./");
      bool project_loaded = false;
      std::string lastArg = argv[0];
      for(int i=1; i<argc; i++)
      {
         if(lastArg == "--project")
         {
            projectSettings.LoadProject(argv[i]);
            project_loaded = true;
         }
         lastArg = argv[i];
      }
      if(!project_loaded)
         projectSettings.LoadProject("./");
      

      m_Window.CreateWindow(
         projectSettings._window.WindowWidth,
         projectSettings._window.WindowHeight,
         projectSettings._application.Name
      );
      m_Window.SetVideoSize(
         projectSettings._window.VideoWidth,
         projectSettings._window.VideoHeight
      );


      for(const std::string& path : projectSettings._modules.modules)
      {
         LoadModule(path, 1);
      }
      #ifdef EASE_EDITOR
         LoadModule("abs://Modules/Ease.Editor", 1);
      #endif


      IMGUI_CHECKVERSION();
      SetupRLImGui(true);
      ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;

      InitModules();
      if(projectSettings._application.MainScene != "")
         m_pCurrentScene->LoadFromFile(projectSettings._application.MainScene.c_str());

      #ifndef EASE_EDITOR
         StartGame();
      #endif
      

      while (!m_Window.ShouldClose())
      {
         Camera2D cam2d;
         {
            auto& cam2dTransform = m_pCurrentScene->CurrentCameraTransform2D();
            auto& cam2dcamera = m_pCurrentScene->CurrentCamera2D();

            cam2d.zoom = cam2dcamera.Zoom() * ((cam2dTransform.Scale().x + cam2dTransform.Scale().y) * 0.5f);
            cam2d.target.x = cam2dTransform.Position().x;
            cam2d.target.y = -cam2dTransform.Position().y;
            cam2d.offset = {m_Window.GetVideoWidth() * 0.5f, m_Window.GetVideoHeight() * 0.5f};
            cam2d.rotation = cam2dTransform.Rotation();
         }
         
         m_Window.BeginPicking();
         BeginMode2D(cam2d);
         Ease::Systems::ProcessAll(m_pCurrentScene, SystemsFlags::Update_PickingDraw);
         Ease::Systems::ProcessAll(m_pCurrentScene, SystemsFlags::Update_Draw, true);
         EndMode2D();
         m_Window.EndPicking();


         m_Window.Begin();
         BeginMode2D(cam2d);
         Ease::Systems::ProcessAll(m_pCurrentScene, SystemsFlags::Update_Draw);
         EndMode2D();

         if(m_AppRunning)
         {
            UpdateGame();
            Ease::Systems::ProcessAll(m_pCurrentScene, SystemsFlags::Update_Logic);
         }
         UpdateModules();


         if(IsKeyPressed(KEY_ENTER))
         {
            std::string dialogPath = Dialog::OpenFileDialog("Title", "", 0, {""}, false);
            std::cout << dialogPath << std::endl;
         }

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

      Ease::ProjectSettings::get_singleton().debug_draw = false;
      Scene::CopyScene(m_GameScene, m_CopyScene);

      m_pCurrentScene->StartScene();
   }


   void Application::UpdateGame()
   {
      m_pCurrentScene->UpdateScene();
   }

   void Application::StopGame()
   {
      if(!m_AppRunning) return;
      m_AppRunning = false;

      Ease::ProjectSettings::get_singleton().debug_draw = true;
      Scene::CopyScene(m_CopyScene, m_GameScene);

      m_pCurrentScene->StopScene();
   }

   void Application::ChangeScene(const char* path)
   {
      m_pCurrentScene->LoadFromFile(path);
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

   Application::ModuleLoadResult Application::LoadModule(const std::filesystem::path& modulePath, int minimumVersion)
   {
      static ResourceManager<NativeModule>& moduleLoader = ResourceManager<NativeModule>::GetLoader();

      std::shared_ptr<NativeModule> myModule = moduleLoader.LoadResource(modulePath.c_str());
      
      // if(author != myModule->m_pModule->metadata.authorName || moduleName != myModule->m_pModule->metadata.moduleName)
      //    return ModuleLoadResult::UNDEFINED_MODULE;
      if(myModule->m_pModule->metadata.version < minimumVersion)
         return ModuleLoadResult::OUTDATED_VERSION;

      for(auto& [name, behaviour] : myModule->m_pModule->nativeBehaviours)
      {
         AddNativeBehaviour(modulePath.filename().concat(".").concat(name), behaviour);
      }
      
      AddModule(modulePath.filename(), myModule);
      return ModuleLoadResult::SUCCESS;
   }

   void Application::AddModule(const std::string& name, std::shared_ptr<NativeModule> _module)
   {
      m_Modules[name] = _module;
   }
   
   void Application::AddNativeBehaviour(const std::string& name, NativeBehaviourFactory* behaviour)
   {
      m_NativeBehaviours[name] = behaviour;
   }
   std::shared_ptr<NativeModule> Application::GetModule(const std::string& name)
   {
      if(m_Modules.count(name) == 1)
         return m_Modules[name];
      else
         return nullptr;
   }

   Ease::NativeBehaviourFactory* Application::GetFactory(const std::string& name)
   {
      if(m_NativeBehaviours.count(name) == 1)
         return m_NativeBehaviours[name];
      else
         return nullptr;
   }
   
   
} // namespace Ease