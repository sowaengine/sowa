#include "Debug.hpp"


void Ease::Debug::test_and_throw(bool expr, const std::string& message)
{
   test_and_throw(expr, message.c_str());
}
void Ease::Debug::test_and_throw(bool expr, const char* message)
{
   if(expr)
      throw std::runtime_error(message);
}