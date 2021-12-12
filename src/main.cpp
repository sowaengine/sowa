#include <iostream>

#include "Resource/Project/Project.hpp"
#include "Application/Application.hpp"


int main(int argc, char const **argv)
{
   std::cout << "\nEase Engine" << std::endl;
   

   Ease::Project.InitProject(argv[0]);
   
   
   try {
      Ease::Application.InitApp();
      Ease::Application.Run();
   }
   catch(const std::exception& e) {
      std::cerr << e.what() << '\n';
      return EXIT_FAILURE;
   }
   
   
   return EXIT_SUCCESS;
}
