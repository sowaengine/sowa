/**
 * @file Shader.hpp
 * @author Lexographics
 * @brief 
 * @version 0.1
 * @date 2021-12-19
 */
#pragma once

#include "Debug.hpp"
#include "glad/glad.h"
#include <glm/glm.hpp>

namespace Ease
{

class Shader
{
   public:
      void CreateShader(const char* filepath);

      void Bind();
      void Unbind();

      void BindUniform(const char* name, const glm::mat4& uniform);
      void BindUniform(const char* name, const glm::vec4& uniform);
      void BindUniform(const char* name, int uniform);

   private:
      GLuint m_ProgramID;
};
}