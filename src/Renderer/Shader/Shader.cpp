#include "Shader.hpp"

#include <fstream>
#include <sstream>
#include <iostream>
#include <cassert>

#include "Renderer/gl/glRenderer.hpp"

// value_ptr
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/ext.hpp>

#include "Utils/File.hpp"

std::string get_file_contents(const char* filename)
{std::cout << filename << std::endl;
   std::ifstream in(filename, std::ios::binary);
   if(in)
   {
      std::string contents;
      in.seekg(0, std::ios::end);
      contents.resize(in.tellg());
      in.seekg(0, std::ios::beg);
      in.read(&contents[0], contents.size());
      in.close();
      //std::cout << contents << std::endl << "--------------------------------" << std::endl;
      return contents;
   }
   assert(false && "Couldn't load file contents");
   throw(errno);
}

namespace Ease
{

static void getShaderError(GLuint shader, const char* shaderType)
{
   GLint result;
   glGetShaderiv(shader, GL_COMPILE_STATUS, &result);
   if (result == GL_FALSE)
   {
      int length;
      glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);

      GLchar* strInfoLog = new GLchar[length + 1];
      glGetShaderInfoLog(shader, length, &length, strInfoLog);
      
      std::cout << "Shader Error " << shaderType << " Shader -> " << strInfoLog << std::endl;
      exit(1);
      delete[] strInfoLog;
   }
}

void Shader::createShader(const char* vertexFile, const char* fragFile)
{
   std::string _vertexSrc = get_file_contents( File::resolveFilepath( vertexFile).fullPath.c_str() );
   std::string _fragSrc =   get_file_contents( File::resolveFilepath( fragFile  ).fullPath.c_str() );

   const char* vertexSrc = _vertexSrc.c_str();
   const char* fragSrc = _fragSrc.c_str();

   gl(GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER));
   gl(glShaderSource(vertexShader, 1, &vertexSrc, nullptr));
   gl(glCompileShader(vertexShader));
   getShaderError(vertexShader, "vertex");
   

   gl(GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER));
   gl(glShaderSource(fragmentShader, 1, &fragSrc, nullptr));
   gl(glCompileShader(fragmentShader));
   getShaderError(fragmentShader, "fragment");


   gl(m_ProgramID = glCreateProgram());
   gl(glAttachShader(m_ProgramID, vertexShader));
   gl(glAttachShader(m_ProgramID, fragmentShader));
   gl(glLinkProgram(m_ProgramID));
   gl(glValidateProgram(m_ProgramID));


   gl(glDeleteShader(vertexShader));
   gl(glDeleteShader(fragmentShader));
}

void Shader::bind()
{
   gl(glUseProgram(m_ProgramID));
}

void Shader::bindUniform(const char* name, const glm::mat4& uniform) {
   bind();
   glUniformMatrix4fv(glGetUniformLocation(m_ProgramID, name), 1, GL_FALSE, glm::value_ptr(uniform));
}
void Shader::bindUniform(const char* name, const glm::vec4& uniform) {
   bind();
   glUniform4fv(glGetUniformLocation(m_ProgramID, name), 1, glm::value_ptr(uniform));
}
void Shader::bindUniform(const char* name, int uniform) {
   bind();
   glUniform1i(glGetUniformLocation(m_ProgramID, name), uniform);
}
   
} // namespace Ease
