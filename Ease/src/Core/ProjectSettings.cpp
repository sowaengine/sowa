#include "Core/ProjectSettings.hpp"

#include "yaml-cpp/yaml.h"

#include <fstream>
#include <sstream>
#include "Debug.hpp"
#include "raylib.h"
#include <iostream>
#include "Utils/File.hpp"


namespace Ease
{
   ProjectSettings::ProjectSettings()
   {

   }

   ProjectSettings::~ProjectSettings()
   {

   }

   bool ProjectSettings::LoadProject(const char* path)
   {
      projectpath = path;

      YAML::Node project = YAML::LoadFile(projectpath / "project.ease");

      Ease::File::InsertFilepathEndpoint("res", projectpath, true);

      int version = project["v"].as<int>(0); 
      
      if(version == 1)
      {
         if(project["Window"])
         {
            YAML::Node __window = project["Window"];
            
            _window.WindowWidth = __window["WindowWidth"].as<int>(_window.WindowWidth);
            _window.WindowHeight = __window["WindowHeight"].as<int>(_window.WindowHeight);
            _window.VideoWidth = __window["VideoWidth"].as<int>(_window.VideoWidth);
            _window.VideoHeight = __window["VideoHeight"].as<int>(_window.VideoHeight);
            _window.Fullscreen = __window["Fullscreen"].as<bool>(_window.Fullscreen);
         }

         if(project["Application"])
         {
            YAML::Node __application = project["Application"];

            _application.Name = __application["Name"].as<std::string>(_application.Name);
            _application.Description = __application["Description"].as<std::string>(_application.Description);
            _application.MainScene = __application["MainScene"].as<std::string>(_application.MainScene);
         }

         if(project["Modules"])
         {
            YAML::Node __modules = project["Modules"];

            _modules.modules = __modules["Modules"].as<std::vector<std::string>>();
         }
      }
      return true;
   }

   bool ProjectSettings::SaveProject()
   {
      static const int version = 1;

      YAML::Emitter yaml;
      
      yaml << YAML::Comment("Ease Engine project file");
      yaml << YAML::Newline;
      yaml << YAML::Comment("Do not modify");
      yaml << YAML::Newline;

      yaml << YAML::BeginMap;
         yaml << YAML::Key << "v" << YAML::Value << version;
         yaml << YAML::Newline << YAML::Newline;
         

         /**  <Application> */
         yaml << YAML::Key << "Application";
         yaml << YAML::Value << YAML::BeginMap;
            yaml << YAML::Key << "Name" << YAML::Value << _application.Name;
            yaml << YAML::Key << "Description" << YAML::Value << _application.Description;
            yaml << YAML::Key << "MainScene" << YAML::Value << _application.MainScene;
            yaml << YAML::Newline;
         yaml << YAML::EndMap << YAML::Newline;
         /** </Application> */


         /**  <Window> */
         yaml << YAML::Key << "Window";
         yaml << YAML::Value << YAML::BeginMap;
            yaml << YAML::Key << "WindowWidth" << YAML::Value << _window.WindowWidth;
            yaml << YAML::Key << "WindowHeight" << YAML::Value << _window.WindowHeight;
            yaml << YAML::Key << "VideoWidth" << YAML::Value << _window.VideoWidth;
            yaml << YAML::Key << "VideoHeight" << YAML::Value << _window.VideoHeight;
            yaml << YAML::Key << "Fullscreen" << YAML::Value << _window.Fullscreen;
            yaml << YAML::Newline;
         yaml << YAML::EndMap;
         /** </Window> */

         /**  <Modules> */
         yaml << YAML::Key << "Modules";
         yaml << YAML::Value << YAML::BeginMap;
            yaml << YAML::Key << "Modules" << YAML::Value << _modules.modules;
         yaml << YAML::EndMap;
         /** </Modules> */
      yaml << YAML::EndMap;

      std::ofstream ofstream(projectpath / "project.ease");
      ofstream << yaml.c_str();
      return true;
   }
} // namespace Ease