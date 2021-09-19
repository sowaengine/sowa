#include "file.hpp"

#include <iostream>

namespace Ease
{
   std::string resPath = "";

   std::string File::getPath(const std::string& path)
   {
      std::string head = path.substr(0, path.find("://"));
      std::string body = path.substr( path.find("://")+3, path.size()-1 );

      
      if(head == "res"){
         std::cout << resPath+body << std::endl;
         return resPath + body;
      }
      else if(head == ""){

      }



      return path;
   }
}