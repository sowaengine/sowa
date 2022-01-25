/**
 * @file File.cpp
 * @author Lexographics
 * @brief 
 * @version 0.1
 * @date 2022-01-26
 */
#include "File.hpp"

#include <fstream>
#include <sstream>
#include <iostream>
#include <cassert>

namespace Ease::Utils
{

   std::string GetFileContents(const char* filename)
   {
      std::ifstream in(filename, std::ios::binary);
      if(in)
      {
         std::string contents;
         in.seekg(0, std::ios::end);
         contents.resize(in.tellg());
         in.seekg(0, std::ios::beg);
         in.read(&contents[0], contents.size());
         in.close();
         return contents;
      }
      assert(false && "Couldn't load file contents" && filename);
      throw(errno);
   }
   
} // namespace Ease::Utils