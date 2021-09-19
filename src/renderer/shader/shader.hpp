#ifndef __SHADER_HPP__
#define __SHADER_HPP__


#include <string>


#include "glad/glad.h"

namespace Ease
{
   class Shader
   {
      private:
         GLuint m_ProgramID;

      public:
         GLuint loadShader(const std::string& filepath);

         GLuint getID() { return m_ProgramID; }
   };
}

#endif // __SHADER_HPP__