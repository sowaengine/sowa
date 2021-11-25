#pragma once


#include "glad/glad.h"
#include <string>

#include <glm/glm.hpp>

namespace Ease
{

class Shader
{
public:
   void createShader(const char* vertexFile, const char* fragFile);

   void bind();

   //GLuint getUniformLocation(const char* location) { return glGetUniformLocation(m_ProgramID, location); }

   void bindUniform(const char* name, const glm::mat4& uniform);
   void bindUniform(const char* name, const glm::vec4& uniform);
   void bindUniform(const char* name, int uniform);
   
   GLuint getID() { return m_ProgramID; }
private:
   GLuint m_ProgramID;
};
   
} // namespace Ease
