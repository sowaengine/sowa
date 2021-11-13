#pragma once


#include <cassert>
#include <stdexcept>
#include <string>
#include <iostream>

#define ASSERT(x) if(!(x)) assert(false)


namespace Ease
{
   namespace Debug
   {
      void test_and_throw(bool expr, const std::string& message);
      void test_and_throw(bool expr, const char* message);

      void log(const std::string& message);
   } // namespace Debug
   
   
   class cl_Stats
   {
   public:
      uint32_t drawCalls;
   };
   extern cl_Stats* stats;
   

} // namespace Ease