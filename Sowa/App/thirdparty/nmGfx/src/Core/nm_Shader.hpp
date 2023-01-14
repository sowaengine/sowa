#ifndef __NM_GFX_SHADER_HPP__
#define __NM_GFX_SHADER_HPP__
#pragma once

#include <string>
#include "glm/glm.hpp"

namespace nmGfx
{
    class Texture;

    class Shader
    {
    public:
        void LoadFile(const char* file);

        void LoadText(const std::string& text);
        void Use();

        
        void UniformFloat(const std::string& name, float value);
        void UniformVec3(const std::string& name, glm::vec3 value);
        void UniformVec4(const std::string& name, glm::vec4 value);
        void UniformMat4(const std::string& name, glm::mat4 value);
        void UniformInt(const std::string& name, int value);
        void UniformTexture(const std::string& name, Texture& texture, int slot);
        void UniformTexture(const std::string& name, unsigned int textureID, int slot);

        Shader();
        ~Shader();
    private:
        std::string _shaderName{""};

        std::string _vertexSource{""};
        std::string _fragmentSource{""};

        unsigned int _vertexShaderID = 0;
        unsigned int _fragmentShaderID = 0;
        unsigned int _programID = 0;

        friend class Renderer;
    };
} // namespace nmGfx


#endif // __NM_GFX_SHADER_HPP__