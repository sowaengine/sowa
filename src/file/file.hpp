#ifndef __FILE_HPP__
#define __FILE_HPP__

#include <string>


namespace Ease
{
   namespace File
   {
      /**
       * "res://images/file.png" -> "home/user/execpath/images/file.png"
       * 
       * @returns absolute path
       **/
      std::string getPath(const std::string& path);
   }
}

#endif // __FILE_HPP__