#include "Shader.hpp"

#include <sstream>

// value_ptr
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/ext.hpp>

#include "Global.hpp"
#include "Resource/Project/Project.hpp"

#include "Utils/File.hpp"


namespace Ease
{  
   static void GetShaderError(GLuint shader, const char* shaderType);
   static void GetProgramError(GLuint program);

   enum class ShaderType
   {
      NONE = 0,
      VERTEX,
      FRAGMENT
   };


   void Shader::CreateShader(const char* filepath)
   {
      std::string relativePath = Global::Project->GetAbsolutePath(filepath);

      std::string shaderCode = Utils::GetFileContents(relativePath.c_str());
      ShaderType currentType = ShaderType::NONE;

      std::string vertexCode = "";
      std::string fragmentCode = "";


      std::stringstream ss(shaderCode);
      std::string line = "";
      while(std::getline(ss, line, '\n'))
      {
         if(line == "#shader vertex")
         {
            currentType = ShaderType::VERTEX;
            continue;
         }
         if(line == "#shader fragment")
         {
            currentType = ShaderType::FRAGMENT;
            continue;
         }
         if(line == "#shader geometry")
         {
            //currentType = ShaderType::GEOMETRY;
            LOG("ERROR: %s", "Geometry shaders are not supported. Ignoring")
            continue;
         }



         if(currentType == ShaderType::VERTEX)
         {
            vertexCode += line + '\n';
         }
         else if(currentType == ShaderType::FRAGMENT)
         {
            fragmentCode += line + '\n';
         }
      }

      const char* vertexSource = vertexCode.c_str();
      const char* fragmentSource = fragmentCode.c_str();
      
      GLuint m_VertexShader = glCreateShader(GL_VERTEX_SHADER);
      glShaderSource(m_VertexShader, 1, &vertexSource, NULL);
      glCompileShader(m_VertexShader);

      GLuint m_FragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
      glShaderSource(m_FragmentShader, 1, &fragmentSource, NULL);
      glCompileShader(m_FragmentShader);

      GetShaderError(m_VertexShader, "Vertex");
      GetShaderError(m_FragmentShader, "Fragment");

      m_ProgramID = glCreateProgram();

      glAttachShader(m_ProgramID, m_VertexShader);
      glAttachShader(m_ProgramID, m_FragmentShader);
      
      glLinkProgram(m_ProgramID);
      GetProgramError(m_ProgramID);

      glDeleteShader(m_VertexShader);
      glDeleteShader(m_FragmentShader);
   }

   void Shader::Bind()
   {
      glUseProgram(m_ProgramID);
   }
   void Shader::Unbind()
   {
      glUseProgram(0);
   }


   void Shader::BindUniform(const char* name, const glm::mat4& uniform) {
      Bind();
      glUniformMatrix4fv(glGetUniformLocation(m_ProgramID, name), 1, GL_FALSE, glm::value_ptr(uniform));
   }
   void Shader::BindUniform(const char* name, const glm::vec4& uniform) {
      Bind();
      glUniform4fv(glGetUniformLocation(m_ProgramID, name), 1, glm::value_ptr(uniform));
   }
   void Shader::BindUniform(const char* name, int uniform) {
      Bind();
      glUniform1i(glGetUniformLocation(m_ProgramID, name), uniform);
   }


   //static
   void GetShaderError(GLuint shader, const char* shaderType)
   {
      GLint result;
      glGetShaderiv(shader, GL_COMPILE_STATUS, &result);
      if (result == GL_FALSE)
      {
         int length;
         glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);

         GLchar* strInfoLog = new GLchar[length + 1];
         glGetShaderInfoLog(shader, length, &length, strInfoLog);
         
         LOG("Shader Error %s Shader -> %s", shaderType, strInfoLog);
         
         exit(1);
         delete[] strInfoLog;
      }
   }

   // static 
   void GetProgramError(GLuint program)
   {
      GLint result;
      glGetProgramiv(program, GL_LINK_STATUS, &result);
      if (result == GL_FALSE)
      {
         int length;
         glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);

         GLchar* strInfoLog = new GLchar[length + 1];
         glGetProgramInfoLog(program, length, &length, strInfoLog);
         
         LOG("Shader Error %s Shader -> %s", "Link Status.", strInfoLog);
         
         exit(1);
         delete[] strInfoLog;
      }
   }
} // namespace Ease
