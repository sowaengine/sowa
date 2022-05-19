#include "ProjectSettings.h"

#include "yaml-cpp/yaml.h"

#include <fstream>
#include <sstream>
#include "Debug.hpp"
#include "raylib.h"


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
      //std::ifstream ifstream(path);
      //std::stringstream str;
      //str << ifstream.rdbuf();

      YAML::Node project = YAML::LoadFile(path);


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
         }
      }
      return true;
   }

   bool ProjectSettings::SaveProject(const char* path)
   {
      static const int version = 1;

      YAML::Emitter yaml;
      
      yaml << YAML::Comment("Ease Engine project file");
      yaml << YAML::Newline;
      yaml << YAML::Comment("Do not modify");
      yaml << YAML::Newline;

      yaml << YAML::BeginMap;
         yaml << YAML::Key << "v" << YAML::Value << version;
         

         /**  <Application> */
         yaml << YAML::Key << "Application";
         yaml << YAML::Value << YAML::BeginMap;
            yaml << YAML::Key << "Name" << YAML::Value << _application.Name;
            yaml << YAML::Key << "Description" << YAML::Value << _application.Description;
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
         yaml << YAML::EndMap << YAML::Newline;
         /** </Window> */
      yaml << YAML::EndMap;

      std::ofstream ofstream(path);
      ofstream << yaml.c_str();
      return true;
   }
} // namespace Ease