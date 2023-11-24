#include "render_layer.hxx"

#include "gl.hxx"
#include <string.h>
#include <vector>

RenderLayer::~RenderLayer() {
	Delete();
}

void RenderLayer::Create(int width, int height) {
	Delete();

	m_width = width;
	m_height = height;

	glGenFramebuffers(1, &m_id);
	glBindFramebuffer(GL_FRAMEBUFFER, m_id);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	for (auto &[slot, target] : m_targets) {
		glGenTextures(1, &target.textureID);
		glBindTexture(GL_TEXTURE_2D, target.textureID);

		if (target.type == RenderLayerTargetType::Vec4) {
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, m_width, m_height, 0, GL_RGBA, GL_FLOAT, NULL);
		} else if (target.type == RenderLayerTargetType::Int) {
			glTexImage2D(GL_TEXTURE_2D, 0, GL_R32I, m_width, m_height, 0, GL_RED_INTEGER, GL_INT, NULL);
		}

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + slot, GL_TEXTURE_2D, target.textureID, 0);
	}

	glGenRenderbuffers(1, &m_rbo);
	glBindRenderbuffer(GL_RENDERBUFFER, m_rbo);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_rbo);

	std::vector<GLenum> attachments;
	for (auto &[slot, target] : m_targets) {
		if (target.type == RenderLayerTargetType::Vec4 || target.type == RenderLayerTargetType::Int) {
			attachments.push_back(GL_COLOR_ATTACHMENT0 + slot);
		} else {
			attachments.push_back(GL_NONE);
		}
	}
	glDrawBuffers(attachments.size(), attachments.data());
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RenderLayer::Resize(int width, int height) {
	Delete();
	Create(width, height);
}

void RenderLayer::Delete() {
	if (m_id != 0) {
		glDeleteFramebuffers(1, &m_id);
	}

	for (auto &[slot, target] : m_targets) {
		glDeleteTextures(1, &target.textureID);
		target.textureID = 0;
	}

	m_id = 0;
}

void RenderLayer::SetTarget(uint32_t slot, renderlayer_type_t type) {
	RenderLayerTarget target;
	target.type = type;

	m_targets[slot] = target;
}

void RenderLayer::Bind() {
	glBindFramebuffer(GL_FRAMEBUFFER, m_id);
	glViewport(0, 0, m_width, m_height);
}

void RenderLayer::Unbind() {
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

int RenderLayer::GetTargetTextureID(int slot) {
	if (m_targets.count(slot) != 0) {
		return m_targets[slot].textureID;
	}

	return 0;
}

void RenderLayer::Clear(float r, float g, float b, float a, bool depth) {
	std::vector<GLenum> attachments;
	for (auto &[slot, target] : m_targets) {
		if (target.type == RenderLayerTargetType::Vec4) {
			attachments.push_back(GL_COLOR_ATTACHMENT0 + slot);
		}
	}

	glDrawBuffers(attachments.size(), attachments.data());
	glClearColor(r, g, b, a);
	glClear(GL_COLOR_BUFFER_BIT);
	if (depth)
		glClear(GL_DEPTH_BUFFER_BIT);

	// FIXME - Web
	GLenum color = GL_COLOR_ATTACHMENT1;
	glDrawBuffers(1, &color);
	glClearColor(0.f, 0.f, 0.f, 0.f);
	glClear(GL_COLOR_BUFFER_BIT);

	attachments.clear();
	for (auto &[slot, target] : m_targets) {
		if (target.type == RenderLayerTargetType::Vec4 || target.type == RenderLayerTargetType::Int) {
			attachments.push_back(GL_COLOR_ATTACHMENT0 + slot);
		} else {
			attachments.push_back(GL_NONE);
		}
	}
	glDrawBuffers(attachments.size(), attachments.data());
}

int RenderLayer::ReadAttachmentInt(int slot, int x, int y) {
	int color = 0;
	glReadBuffer(GL_COLOR_ATTACHMENT0 + slot);
	glReadPixels(x, m_height - y, 1, 1, GL_RED_INTEGER, GL_INT, &color);
	return color;
}