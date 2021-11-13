#include "Debug.hpp"

#ifdef EASE_EDITOR
   #include "Editor/Editor.hpp"
#endif

namespace Ease
{

void Debug::test_and_throw(bool expr, const std::string& message) {
   test_and_throw(expr, message.c_str());
}
void Debug::test_and_throw(bool expr, const char* message)
{
   if(!expr)
   {
      throw std::runtime_error(message);
   }
}

void Debug::log(const std::string& message)
{
   #ifdef EASE_EDITOR
      Editor::printLine(message);
   #else
      std::cout << "LOG: " << message << std::endl;
   #endif
}

   
} // namespace Ease
