#pragma once

#include "glad/glad.h"
#include <GLFW/glfw3.h>

#include <string>

namespace Ease
{

class Window
{
public:
   Window();
   void create(int width, int height, const char* title);

   GLFWwindow* get() { return m_WindowPointer; }

   bool shouldClose() { return glfwWindowShouldClose(m_WindowPointer); }

   void getWindowSize(int* width, int* height) { *width = m_Width; *height = m_Height; }
   void setWindowSize(int  width, int  height) { m_Width = width ; m_Height = height; }

   void getVideoSize(int* width, int* height) { *width = m_VideoWidth;  *height = m_VideoHeight; }
   void setVideoSize(int  width, int  height) { m_VideoWidth = width ; m_VideoHeight = height; }

   void setTitle(const std::string& title) { m_Title = title; glfwSetWindowTitle(m_WindowPointer, title.c_str()); }
   void resizeWindow() { glfwSetWindowSize(m_WindowPointer, m_Width, m_Height); }
private:
// actual size of window
   int m_Width;
   int m_Height;

// resolution that we will render
   int m_VideoWidth{1280};
   int m_VideoHeight{720};
   std::string m_Title;

   GLFWwindow* m_WindowPointer;
};
   
} // namespace Ease
