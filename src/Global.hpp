/**
 * @file Global.hpp
 * @author Lexographics
 * @brief Contains Global Variables
 * @version 0.1
 * @date 2021-12-12
 */
#pragma once


// forward declarations of Global Classes
namespace Ease
{
   class cl_Project;
   class cl_ResourceManager;
   class cl_Application;
}

namespace Ease::Global
{
   extern cl_Project* Project;
   extern cl_ResourceManager* ResourceManager;
   extern cl_Application* Application;
} // namespace Ease::Global
