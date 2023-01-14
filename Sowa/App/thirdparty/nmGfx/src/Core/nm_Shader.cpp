#include "nm_Shader.hpp"
#include <fstream>
#include <sstream>
#include "glad/glad.h"
#include "Core/GL/nm_Texture.hpp"

namespace nmGfx
{
    void Shader::LoadFile(const char* file)
    {
        std::ifstream ifstream;
        ifstream.open(file);
        if(!ifstream)
        {
#ifdef NMGFX_PRINT_MESSAGES
            fprintf(stderr, "Failed to load file: %s\n", file);
#endif
            return;
        }

        std::stringstream strstream;
        strstream << ifstream.rdbuf();

        _shaderName = file;
        LoadText(strstream.str());
    }

    static unsigned int GenerateShader(GLenum shaderType, const std::string& src)
    {
        const char* source = src.c_str();

        unsigned int id = glCreateShader(shaderType);
        glShaderSource(id, 1, &source, NULL);
        glCompileShader(id);
#ifdef NMGFX_PRINT_MESSAGES
        {
            int  success;
            char infoLog[512];
            glGetShaderiv(id, GL_COMPILE_STATUS, &success);

            if(!success)
            {
                glGetShaderInfoLog(id, 512, NULL, infoLog);
                if(shaderType == GL_VERTEX_SHADER)
                    printf("%s Shader compilation failed.\n %s\n", "Vertex", infoLog);
                else if(shaderType == GL_FRAGMENT_SHADER)
                    printf("%s Shader compilation failed.\n %s\n", "Fragment", infoLog);
            }
        }
#endif

        return id;
    }
    static unsigned int GenerateProgram(uint32_t vertexShader, uint32_t fragmentShader)
    {
        unsigned int id = glCreateProgram();
        glAttachShader(id, vertexShader);
        glAttachShader(id, fragmentShader);
        glLinkProgram(id);
        
#ifdef NMGFX_PRINT_MESSAGES
        {
            int  success;
            char infoLog[512];
            glGetProgramiv(id, GL_LINK_STATUS, &success);

            if(!success)
            {
                glGetProgramInfoLog(id, 512, NULL, infoLog);
                printf("Shader Program compilation failed. \n %s\n", infoLog);
            }
        }
#endif

        return id;
    }

    void Shader::LoadText(const std::string& text)
    {
        std::istringstream file(text);
        std::string line{""};
        std::string* currentShaderSource = nullptr;
        while(std::getline(file, line))
        {
            if(line.find("#shader vertex", 0) == 0)
            {
                currentShaderSource = &_vertexSource;
                continue;
            }
            if(line.find("#shader fragment", 0) == 0)
            {
                currentShaderSource = &_fragmentSource;
                continue;
            }
            
            if(currentShaderSource != nullptr)
            {
                *currentShaderSource += line + '\n';
            }
        }

        // printf("Loaded Shader.\nVertex Shader: \n%s\n\nFragment Shader: \n%s\n", _vertexSource.c_str(), _fragmentSource.c_str());
        _vertexShaderID = GenerateShader(GL_VERTEX_SHADER, _vertexSource);
        _fragmentShaderID = GenerateShader(GL_FRAGMENT_SHADER, _fragmentSource);

        _programID = GenerateProgram(_vertexShaderID, _fragmentShaderID);

        glDeleteShader(_vertexShaderID);
        glDeleteShader(_fragmentShaderID);
#ifdef NMGFX_PRINT_MESSAGES
        printf("Loaded shader with id: %i, path: %s\n", _programID, _shaderName.c_str());
#endif
    }

    void Shader::Use()
    {
        glUseProgram(_programID);
    }

    Shader::Shader()
    {
    }
    Shader::~Shader()
    {
        glDeleteProgram(_programID);
    }



    // Uniforms (TEMPORARY SOLUTION) // todo add location caching
    void Shader::UniformFloat(const std::string& name, float value)
    {
        Use();
        int loc = glGetUniformLocation(_programID, name.c_str());
        if(loc < 0)
        {
#ifdef NMGFX_PRINT_MESSAGES
            printf("Failed to get uniform location: %s, Program ID: %i\n", name.c_str(), _programID);
#endif
            return;
        }
        glUniform1f(loc, value);
    }

    void Shader::UniformVec3(const std::string& name, glm::vec3 value)
    {
        Use();
        int loc = glGetUniformLocation(_programID, name.c_str());
        if(loc < 0)
        {
#ifdef NMGFX_PRINT_MESSAGES
            printf("Failed to get uniform location: %s, Program ID: %i\n", name.c_str(), _programID);
#endif
            return;
        }
        glUniform3f(loc, value.x, value.y, value.z);
    }
    void Shader::UniformVec4(const std::string& name, glm::vec4 value)
    {
        Use();
        int loc = glGetUniformLocation(_programID, name.c_str());
        if(loc < 0)
        {
#ifdef NMGFX_PRINT_MESSAGES
            printf("Failed to get uniform location: %s, Program ID: %i\n", name.c_str(), _programID);
#endif
            return;
        }
        glUniform4f(loc, value.x, value.y, value.z, value.w);
    }

    void Shader::UniformMat4(const std::string& name, glm::mat4 value)
    {
        Use();
        int loc = glGetUniformLocation(_programID, name.c_str());
        if(loc < 0)
        {
#ifdef NMGFX_PRINT_MESSAGES
            printf("Failed to get uniform location: %s, Program ID: %i\n", name.c_str(), _programID);
#endif
            return;
        }
        glUniformMatrix4fv(loc, 1, GL_FALSE, &value[0][0]);
    }

    void Shader::UniformInt(const std::string& name, int value)
    {
        Use();
        int loc = glGetUniformLocation(_programID, name.c_str());
        if(loc < 0)
        {
#ifdef NMGFX_PRINT_MESSAGES
            printf("Failed to get uniform location: %s, Program ID: %i\n", name.c_str(), _programID);
#endif
            return;
        }
        glUniform1i(loc, value);
    }

    void Shader::UniformTexture(const std::string& name, Texture& texture, int slot)
    {
        Use();
        int loc = glGetUniformLocation(_programID, name.c_str());
        if(loc < 0)
        {
#ifdef NMGFX_PRINT_MESSAGES
            printf("Failed to get uniform location: %s, Program ID: %i\n", name.c_str(), _programID);
#endif
            return;
        }
        texture.Use(slot);
        glUniform1i(loc, slot);
    }

    void Shader::UniformTexture(const std::string& name, unsigned int textureID, int slot)
    {
        Use();
        int loc = glGetUniformLocation(_programID, name.c_str());
        if(loc < 0)
        {
#ifdef NMGFX_PRINT_MESSAGES
            printf("Failed to get uniform location: %s, Program ID: %i\n", name.c_str(), _programID);
#endif
            return;
        }
        glActiveTexture(GL_TEXTURE0+slot);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glUniform1i(loc, slot);
    }
} // namespace nmGfx
