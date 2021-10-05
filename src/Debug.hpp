#pragma once


#include <cassert>
#include <stdexcept>
#include <string>

namespace Ease
{
   namespace Debug
   {
      void test_and_throw(bool expr, const std::string& message);
      void test_and_throw(bool expr, const char* message);

   } // namespace Debug
} // namespace Ease