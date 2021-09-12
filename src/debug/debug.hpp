#ifndef __DEBUG_HPP__
#define __DEBUG_HPP__

#include <string>

namespace Ease
{
   namespace debug
   {
      /**
       * *Logs messages into console
      */
      void log(const char* message, int errorCode = 0);
      void log(const std::string& message, int errorCode = 0);

      /**
       * *Logs warnings into console (color: yellow)
      */
      void logWarning(const char* message, int errorCode = 0);
      void logWarning(const std::string& message, int errorCode = 0);

      /**
       * *Logs errors into console (color: red)
      */
      void logError(const char* message, int errorCode = 0);
      void logError(const std::string& message, int errorCode = 0);
   }
   
}


#endif // __DEBUG_HPP__