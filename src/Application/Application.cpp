#include "Application.hpp"
#include "Debug.hpp"
#include "Ease.hpp"


namespace Ease
{

void Application::Run()
{
   while (!window.shouldClose())
   {
      glfwPollEvents();
   }
   
}

} // namespace Ease