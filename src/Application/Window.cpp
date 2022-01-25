/**
 * @file Window.cpp
 * @author Lexographics
 * @brief 
 * @version 0.1
 * @date 2022-01-26
 */
#include "Window.hpp"  
#include "Debug.hpp"





Window::Window()
{
}
	
Window::~Window()
{
}


void Window::SetClearColor(float r, float g, float b, float a) const
{
   glClearColor(r, g, b, a);
}

void Window::PollEvents() const
{
   glfwPollEvents();
}

void Window::Clear(int bits) const
{
   glClear(bits);
}

void Window::SwapBuffers() const
{
   glfwSwapBuffers(m_WindowPointer);
}

bool Window::WindowShouldClose() const
{
   return glfwWindowShouldClose(m_WindowPointer);
}



void Window::CreateWindow(int _width, int _height, std::string _title) 
{
   static bool windowCreated = false;

   if(windowCreated)
   {
      LOG_ERROR("%s", "Window Already Created");
      throw std::runtime_error("");
   }

   m_Width = _width;
   m_Height = _height;
   m_Title = _title;

   // Create window
   m_WindowPointer = glfwCreateWindow(m_Width, m_Height, m_Title.c_str(), NULL, NULL);
   glfwMakeContextCurrent(m_WindowPointer);


   windowCreated = true;
}

void Window::UpdateWindowSize() const
{
   glfwSetWindowSize(m_WindowPointer, m_Width, m_Height);
}
void Window::UpdateWindowTitle() const
{
   glfwSetWindowTitle(m_WindowPointer, m_Title.c_str());
}


Window& Window::get_singleton() 
{
   static Window window;
   return window;
}
