#include "debug.hpp"

#include <iostream>


namespace Ease
{
   namespace debug
   {
      void log(const char* message, int errorCode) {
         std::cout << "LOG: " << message << " / Error Code: " << errorCode << std::endl;
      }
      void log(const std::string& message, int errorCode) {
         log(message.c_str(), errorCode);
      }


      void logWarning(const char* message, int errorCode) {
         std::cout << "\033[33;1mWARNING: " << "\033[0m" << message << " / Error Code: " << errorCode << std::endl;
      }
      void logWarning(const std::string& message, int errorCode) {
         logWarning(message.c_str(), errorCode);
      }


      void logError(const char* message, int errorCode) {
         std::cout << "\033[31;1mERROR: " << "\033[0m" << message << " / Error Code: " << errorCode << std::endl;
      }
      void logError(const std::string& message, int errorCode) {
         logError(message.c_str(), errorCode);
      }
   }
}
