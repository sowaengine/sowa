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

      
      m_ProgramID = glCreateProgram();

      /* Compile Vertex Shader */
      GLuint vertexID = glCreateShader(GL_VERTEX_SHADER);
      const char* src_vert = vertexShaderSrc.c_str();
      glShaderSource(vertexID, 1, &src_vert, nullptr);
      glCompileShader(vertexID);
      GLint result;
      glGetShaderiv(vertexID, GL_COMPILE_STATUS, &result);
      if(result == GL_FALSE)
      {
         int length;
         glGetShaderiv(vertexID, GL_INFO_LOG_LENGTH, &length);

         GLchar* strInfoLog = new GLchar[length+1];
         glGetShaderInfoLog(vertexID, length, &length, strInfoLog);

         std::cout << "Shader Error Vertex Shader -> " << strInfoLog << std::endl;
         delete[] strInfoLog;
      }

      /* Compile Fragment Shader */
      GLuint fragID = glCreateShader(GL_FRAGMENT_SHADER);
      const char* src_frag = fragmentShaderSrc.c_str();
      glShaderSource(fragID, 1, &src_frag, nullptr);
      glCompileShader(fragID);
      // GLint result;
      glGetShaderiv(fragID, GL_COMPILE_STATUS, &result);
      if(result == GL_FALSE)
      {
         int length;
         glGetShaderiv(fragID, GL_INFO_LOG_LENGTH, &length);

         GLchar* strInfoLog = new GLchar[length+1];
         glGetShaderInfoLog(fragID, length, &length, strInfoLog);

         std::cout << "Shader Error Fragment Shader -> " << strInfoLog << std::endl;
         delete[] strInfoLog;
      }


      /* Link Shaders into program */
      glAttachShader(m_ProgramID, vertexID);
      glAttachShader(m_ProgramID, fragID);
      // attach other shader types if there is

      glLinkProgram(m_ProgramID);
      glValidateProgram(m_ProgramID);
      glUseProgram(m_ProgramID);

      glDeleteShader(vertexID);
      glDeleteShader(fragID);


      std::cout << "Created new Shader Program with id: " << m_ProgramID << std::endl;
      return m_ProgramID;
   }
}