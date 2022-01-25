/**
 * @file ProjectSettings.hpp
 * @author Lexographics
 * @brief 
 * @version 0.1
 * @date 2022-01-13
 */
#ifndef PROJECTSETTINGS_H
#define PROJECTSETTINGS_H
#pragma once

#include <string>
#include "glm/glm.hpp"

struct ProjectSettings
{
   std::string Title;
   glm::vec2 WindowSize;


   ProjectSettings()
      : Title("Ease Engine Game"), WindowSize(1280, 720)
   {
   }
};


#endif