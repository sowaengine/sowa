#include <iostream>

#include "Resource/Project/Project.hpp"
#include "Application/Application.hpp"
#include "Resource/ResourceManager/ResourceManager.hpp"
#include "Scene/SceneTree.hpp"

#include "Debug.hpp"
#include "Global.hpp"

namespace Ease
{
   cl_Project* Global::Project;
   cl_ResourceManager* Global::ResourceManager;
   cl_Application* Global::Application;
   cl_SceneTree* Global::SceneTree;
}

int main(int argc, char const **argv)
{
   std::cout << "\nEase Engine" << std::endl;
   
   Ease::Global::Application->Init();
   Ease::Global::ResourceManager->Init();
   Ease::Global::Project->InitProject(argv[0]);
   Ease::Global::SceneTree->Init();
   
   try {
      Ease::Global::Application->InitApp();
      Ease::Global::Application->Run();
   }
   catch(const std::exception& e) {
      std::cerr << e.what() << '\n';
      return EXIT_FAILURE;
   }
   
   
   return EXIT_SUCCESS;
}
