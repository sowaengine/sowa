/**
 * @file Project.hpp
 * @author Lexographics
 * @brief 
 * @version 0.1
 * @date 2021-12-12
 */

#pragma once

#include <string>
#include <filesystem>


namespace Ease
{

struct ProjectSettings
{
//  APPLICATION
   //  CONFIG
      std::string Title = "Ease Engine";
      std::string Description = "Description";
      std::string IconPath = "res://textures/ship.png";
   // ~CONFIG

   //  RUN
      std::string MainScene = "res://scenes/scene.escn";
   // ~RUN

   //  WINDOW
      // Resolution that will be drawn to screen
      // irrelevant to screen resolution
      uint32_t RenderWidth = 1280;
      uint32_t RenderHeight = 720;

      // Target resolution of window
      uint32_t WindowWidth = 1280;
      uint32_t WindowHeight = 720;

      bool Resizable = true;
      bool FullScreen = false;
   // ~WINDOW
// ~APPLICATION
};


// Singleton class, dont instantiate
class cl_Project
{
public:
   // initializes project on current program directory
   static void InitProject(const std::string& executableName);

   // Returns absolute path of a file that is located in project directory
   std::string GetAbsolutePath(const std::string& relativePath);


   const ProjectSettings& GetProjectSettings() { return m_ProjectSettings; }

   std::string dataDir = "";
private:
   cl_Project() = default;
   void Init(const std::string& executableName);

   std::string m_ProjectDir = "";
   std::string m_ExecutableName = "";

   std::string m_DataDir = "";


   ProjectSettings m_ProjectSettings;
};


} // namespace Ease
