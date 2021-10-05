#include <iostream>
#include "Ease.hpp"
#include "Debug.hpp"

#include "Application/Application.hpp"

int main(int argc, char const **argv)
{
   std::cout << "Ease Engine" << std::endl;
   Ease::Application app;


   try { app.Run(); }
   catch(const std::exception& e)
   {
      std::cerr << e.what() << '\n';
   }
   
   return 0;
}
