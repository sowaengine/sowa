#include "framebuffer_gl.hpp"

#include "gfx/gl/glfuncs.hpp"
#include "debug.hpp"

#include "glad/glad.h"

namespace sowa {
namespace gfx {


GLFramebuffer::~GLFramebuffer() {
	Delete();
}

void GLFramebuffer::Create(int width, int height) {
	Delete();

	m_width = width;
	m_height = height;

    GL().genFramebuffer(m_id);
    GL().bindFramebuffer(m_id);
    GL().setUnpackAlignment(1);

	for (auto &[slot, target] : m_targets) {
        if(target.type == GLFramebufferTargetType::Vec4) {
		    target.texture.Load2DFromData(nullptr, width, height, GLDataType::Float, GLTextureFormat::Rgba, GLTextureInternalFormat::Rgba16F);
            // glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, target.texture.ID(), 0);
            
        } else if(target.type == GLFramebufferTargetType::Uint32) {
            target.texture.Load2DFromData(nullptr, width, height, GLDataType::Int, GLTextureFormat::RedInteger, GLTextureInternalFormat::R32I);
        }
        GL().framebufferTexture2D(slot, target.texture.ID());
	}

    glGenRenderbuffers(1, &m_rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, m_rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_rbo);


    std::vector<int> attachments;
    for (auto &[slot, target] : m_targets) {
        if(target.type == GLFramebufferTargetType::Vec4 || target.type == GLFramebufferTargetType::Uint32) {
            attachments.push_back(slot);
        }
    }
    GL().drawBuffers(attachments);

    if(!GL().checkFramebufferStatus()) {
        Debug::Error("Failed to create framebuffer");
    }

    GL().bindFramebuffer(0);
}
void GLFramebuffer::Delete() {
	if (m_id != 0) {
        GL().deleteFramebuffer(m_id);
	}
	m_id = 0;
}

void GLFramebuffer::SetTarget(uint32_t slot, GLFramebufferTargetType type) {
    GLFramebufferTarget target;
    target.type = type;

    m_targets[slot] = target;
}

void GLFramebuffer::Bind() {
    GL().bindFramebuffer(m_id);
    GL().viewport(0, 0, m_width, m_height);
}
void GLFramebuffer::Unbind() {
    GL().bindFramebuffer(0);
}

int GLFramebuffer::GetTargetTextureID(int slot) {
    if(m_targets.count(slot) != 0) {
        return m_targets[slot].texture.ID();
    }

    return 0;
}

} // namespace gfx
} // namespace sowa
