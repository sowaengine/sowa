/**
 * @file main.cpp
 * @author Lexographics
 * @brief Entry Point
 * @version 0.1
 * @date 2022-01-26
 */
#include <iostream>
#include "Resource/Project.hpp"
#include "Application/Application.hpp"


int main(int argc, char const *argv[])
{
   std::cout << "Ease Engine" << std::endl;

   Project& project = Project::get_singleton();

   project.LoadProject(std::string(argv[0]) + "_Data");
   Application& app = Application::get_singleton();

   try { app.Run(); }
   catch(const std::exception& e) {
      // Todo: Log Error to a log file
      std::cerr << e.what() << '\n';
   }
   


   return 0;
}
