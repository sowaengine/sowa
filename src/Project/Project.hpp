// Project class that contains configurations and settings of project
// Serializer:deserializeScene Returns Project Object
// Serializer:serializeScene takes a project object as argument

#include <map>
#include "Ease.hpp"

namespace Ease
{

// ALL PATHS ARE RELATIVE PATH TO PROJECT PATH
struct ProjectSettings
{
//  APPLICATION
   //  CONFIG
      std::string title = "Ease Engine";
      std::string description = "Description";
      std::string iconPath = "res://textures/ship.png";
   // ~CONFIG

   //  RUN
      std::string mainScene = "res://scenes/scene.escn";
   // ~RUN

   //  WINDOW
      // Resolution that will be drawn to screen
      // irrelevant to screen resolution
      uint32_t renderWidth = 1280;
      uint32_t renderHeight = 720;

      // Target resolution of window
      uint32_t windowWidth = 1920;
      uint32_t windowHeight = 1080;

      bool resizable = true;
      bool fullScreen = false;
      bool useVSync = true;
   // ~WINDOW
// ~APPLICATION
};

 
class Project
{
public:
   ProjectSettings projectSettings;

   std::map<uint32_t, std::string> externalPaths;
   std::string getPathFromExt(uint32_t ext_id);
private:
   std::string projectPath;

   friend class Serializer;
};

} // namespace Ease
