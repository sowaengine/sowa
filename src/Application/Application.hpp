/**
 * @file Application.hpp
 * @author Lexographics
 * @brief 
 * @version 0.1
 * @date 2021-12-12
 */

#pragma once

#include <string>
#include "glad/glad.h"
#include "Window/Window.hpp"


namespace Ease
{


// Singleton class, dont instantiate
class cl_Application
{
public:
   static void Init();


   void InitApp();

   void Run();

   void Quit(int code = 0);

private:
   cl_Application() = default;


   Window m_Window{};

};


} // namespace Ease
