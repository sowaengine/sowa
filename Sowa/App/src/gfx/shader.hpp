#ifndef SW_SHADER_HPP_
#define SW_SHADER_HPP_

#include <string>
#include <cstdint>

#include "math/math.hpp"

namespace sowa {
namespace gfx {
class IShader {
    public:
        virtual void New(const std::string &vsource, const std::string& fsource) = 0;
        virtual void Delete() = 0;

        virtual void Bind() = 0;
        virtual void Unbind() = 0;

        virtual const uint32_t ID() = 0;

        virtual void UniformTexture(const std::string& name, uint32_t textureID, int slot) = 0;
        virtual void UniformMat4(const std::string& name, const mat4& value) = 0;
        virtual void UniformVec2(const std::string& name, const glm::vec2& value) = 0;
        virtual void UniformVec4(const std::string& name, const glm::vec4& value) = 0;
        virtual void UniformIntVector(const std::string& name, const std::vector<int>& value) = 0;
};
} // namespace gfx
} // namespace sowa

#endif // SW_SHADER_HPP_