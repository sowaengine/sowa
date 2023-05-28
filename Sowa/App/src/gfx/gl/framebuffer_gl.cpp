#include "framebuffer_gl.hpp"

#include "debug.hpp"
#include "gfx/gl/glfuncs.hpp"
#include "gfx/graphics.hpp"

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
		if (target.type == GLFramebufferTargetType::Vec4) {
			target.texture.Load2DFromData(nullptr, width, height, GLDataType::Float, GLTextureFormat::Rgba, GLTextureInternalFormat::Rgba16F);

		} else if (target.type == GLFramebufferTargetType::Int) {
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
		if (target.type == GLFramebufferTargetType::Vec4 || target.type == GLFramebufferTargetType::Int) {
			attachments.push_back(slot);
		}
	}
	GL().drawBuffers(attachments);

	if (!GL().checkFramebufferStatus()) {
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
	if (m_targets.count(slot) != 0) {
		return m_targets[slot].texture.ID();
	}

	return 0;
}

int GLFramebuffer::ReadAttachmentInt(int slot, int x, int y) {
	int color = 0;
	glReadBuffer(GL_COLOR_ATTACHMENT0 + slot);
	glReadPixels(x, y, 1, 1, GL_RED_INTEGER, GL_INT, &color);
	return color;
}

void GLFramebuffer::ReadAttachmentColor(int slot, int x, int y, unsigned char color[4]) {
	glReadBuffer(GL_COLOR_ATTACHMENT0 + slot);
	glReadPixels(x, y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, color);
}
ViewportDrawMode &GLFramebuffer::DrawMode() {
	return m_drawMode;
}

} // namespace gfx
} // namespace sowa
