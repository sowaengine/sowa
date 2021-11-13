#include "Serializer.hpp"

#include <sstream>
#include <fstream>
#include <unordered_map>
#include <filesystem>

#include <glm/glm.hpp>
#include "yaml-cpp/yaml.h"

#include "Utils/File.hpp"
#include "Project/Project.hpp"


namespace YAML
{
   template<>
   struct convert<glm::vec2>
   {
      static Node encode(const glm::vec2& item)
      {
         Node node;
         node.push_back(item.x);
         node.push_back(item.y);
         return node;
      }
      static bool decode(const Node& node, glm::vec2& item)
      {
         if(!node.IsSequence() || node.size() != 2)
            return false;
         
         item.x = node[0].as<float>();
         item.y = node[1].as<float>();

         return true;
      }
   };

   // YAML Overloads

   YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec2& item)
   {
      out << YAML::Flow;
      out << YAML::BeginSeq << item.x << item.y << YAML::EndSeq;
      return out;
   }
} // namespace YAML


namespace Ease
{

bool Serializer::DeserializeProject(const std::string& filepath, Project& project)
{
   File::setPath_Head("res", "unknown");
   
   std::ifstream stream(filepath);
   std::stringstream strStream;
   strStream << stream.rdbuf();

   YAML::Node data = YAML::Load(strStream.str());
   
   // find res://
   std::string fullPath = std::filesystem::absolute(filepath);

   std::string directory = "unknown";
   const size_t last_slash_idx = filepath.rfind('/');
   if (std::string::npos != last_slash_idx)
   {
      directory = filepath.substr(0, last_slash_idx) + "/";
   }
   File::setPath_Head("res", directory);
   project.projectPath = directory;
   
   if(data["Application"])
   {
      YAML::Node _application = data["Application"];

      if(_application["Config"])
      {
         YAML::Node _config = _application["Config"];
         if(_config["title"])
            project.projectSettings.title = _config["title"].as<std::string>("Ease Engine Fallback");
         if(_config["description"])
            project.projectSettings.description = _config["description"].as<std::string>("DescriptionFallback");
         if(_config["icon"])
            project.projectSettings.iconPath = _config["icon"].as<std::string>("");
      }
      
      if(_application["Run"])
      {
         YAML::Node _run = _application["Run"];
         if(_run["mainScene"])
            project.projectSettings.mainScene = _run["mainScene"].as<std::string>("");
      }

      if(_application["Window"])
      {
         YAML::Node _window = _application["Window"];
         if(_window["renderWidth"])
            project.projectSettings.renderWidth = _window["renderWidth"].as<uint32_t>(1280);
         if(_window["renderHeight"])
            project.projectSettings.renderHeight = _window["renderHeight"].as<uint32_t>(720);
         if(_window["windowWidth"])
            project.projectSettings.windowWidth = _window["windowWidth"].as<uint32_t>(1366);
         if(_window["windowHeight"])
            project.projectSettings.windowHeight = _window["windowHeight"].as<uint32_t>(768);
         if(_window["resizeable"])
            project.projectSettings.resizable = _window["resizeable"].as<bool>(true);
         if(_window["fullScreen"])
            project.projectSettings.fullScreen = _window["fullScreen"].as<bool>(true);
         if(_window["useVSync"])
            project.projectSettings.useVSync = _window["useVSync"].as<bool>(true);
      }
   }

   // load external paths
   if(data["ext"])
   {
      YAML::Node _ext = data["ext"];
      for (YAML::const_iterator it = _ext.begin(); it != _ext.end(); ++it)
      {
         uint32_t id = it->first.as<unsigned int>(0);
         std::string path = it->second.as<std::string>("");
         project.externalPaths[id] = path;
      }
   }

   return true;
}
   
} // namespace Ease

