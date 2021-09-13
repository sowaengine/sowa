#include "utils.hpp"

#include <chrono>
#include <random>

namespace Ease
{
   uint32_t Util::generateUUID()
   {
      for(int i=0; i<time(NULL)%22; i++) {
         rand();
      }
      return rand();
   }
}