#ifndef SW_FRAMEBUFFER_GL_HPP_
#define SW_FRAMEBUFFER_GL_HPP_

#include <map>
#include <cstdint>
#include "texture_gl.hpp"

namespace sowa {
namespace gfx {

enum class GLFramebufferTargetType {
    Vec4,
    Uint32,
    // Depth24_Stencil8,
};

struct GLFramebufferTarget {
    GLFramebufferTargetType type;
    GLTexture texture;
};

class GLFramebuffer {
    public:
        GLFramebuffer() = default;
        ~GLFramebuffer();

        void Create(int width, int height);
        void Delete();

        void SetTarget(uint32_t slot, GLFramebufferTargetType type);

        void Bind();
        void Unbind();
    private:
        // slot, target
        std::map<uint32_t, GLFramebufferTarget> m_targets;
        int m_width = 0;
        int m_height = 0;

        uint32_t m_id = 0;
};
} // namespace gfx
} // namespace sowa

#endif // SW_FRAMEBUFFER_GL_HPP_