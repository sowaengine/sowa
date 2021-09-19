#include "shader.hpp"

#include <fstream>
#include <istream>
#include <sstream>
#include <iostream>

#include "debug/debug.hpp"

namespace Ease
{
   enum ShaderType
   {
      NONE,
      VERTEX,
      FRAGMENT,
   };

   bool isShaderType(const std::string& line) {
      if (line == "#shader vert")
         return true;
      if (line == "#shader frag")
         return true;
      
      return false;
   }
   ShaderType getShaderType(const std::string& line) {
      if (line == "#shader vert")
         return ShaderType::VERTEX;
      if (line == "#shader frag")
         return ShaderType::FRAGMENT;
      
      return ShaderType::NONE;
   }


   GLuint Shader::loadShader(const std::string& filepath)
   {
      std::string vertexShaderSrc = "";
      std::string fragmentShaderSrc = "";


      std::ifstream ifstream;

      ifstream.open(filepath);
      if(!ifstream) {
         std::cout << "Unable to open file: " << filepath << std::endl;
         return 0;
      }

      std::string line = "";
      ShaderType currentShaderType;

      while ( getline(ifstream, line ) ) {
         if(isShaderType(line))
         {
            currentShaderType = getShaderType(line);

         } else {
            switch (currentShaderType)
            {
               case ShaderType::VERTEX:
                  vertexShaderSrc += line + '\n';
                  break;
               case ShaderType::FRAGMENT:
                  fragmentShaderSrc += line + '\n';
                  break;

            default:
               break;
            }
         }


      }
      ifstream.close();

      if(vertexShaderSrc == "") {
         Ease::debug::logError("Vertex Shader Not Given");
         return 0;
      }
      if(fragmentShaderSrc == "") {
         Ease::debug::logError("Fragment Shader Not Given");
         return 0;
      }

      std::cout << "Vertex Shader:\n"
                << vertexShaderSrc << "\n"
                << "Fragment Shader: \n"
                << fragmentShaderSrc << std::endl;

      return 0;
   }
}