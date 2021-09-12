#include <iostream>

#include "renderer/window/window.hpp"
#include "core/engine.hpp"
#include "global.hpp"

Ease::Engine* EngineRef;


int main(int argc, char const **argv)
{
   std::cout << "Ease Engine" << std::endl;
   
   Ease::Window window("Ease Engine", 1280, 720);
   windowRef = &window;

   Ease::Engine engine;
   EngineRef = &engine;

   // Main Loop
   unsigned int exitError = 0;
   while ( exitError = engine.Update() )
   {
      
   }
   
   
   return exitError;
}
