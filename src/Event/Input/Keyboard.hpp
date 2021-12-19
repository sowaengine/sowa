/**
 * @file Keyboard.hpp
 * @author Lexographics
 * @brief 
 * @version 0.1
 * @date 2021-12-19
 */
#pragma once

#include "Debug.hpp"
#include <unordered_map>
#include <GLFW/glfw3.h>

enum class KeyCode;

namespace Ease::Input
{
   void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);


   bool IsKeyPressed(KeyCode key);
}