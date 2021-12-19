/**
 * @file Window.hpp
 * @author Lexographics
 * @brief 
 * @version 0.1
 * @date 2021-12-12
 */
#pragma once

#include <string>
#include <GLFW/glfw3.h>


namespace Ease
{

class Window
{
public:
   void Create(int width, int height, const std::string& title);


   GLFWwindow* GetPtr() { return m_WindowPointer; }

   inline uint16_t WindowWidth() { return m_WindowWidth; }
   inline uint16_t WindowHeight() { return m_WindowHeight; }

   
private:
   GLFWwindow* m_WindowPointer = nullptr;

   uint16_t m_WindowWidth;
   uint16_t m_WindowHeight;
   
   friend class WindowResizeUpdater;
};
   
} // namespace Ease
