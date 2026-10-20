#include "render/gl/gl_render_target.hpp"

#include "render/gl/gl.hpp"
#include "render/gl/gl_texture.hpp"

#include "core/log.hpp"

namespace sowa {

RenderTargetHandle GLRenderTargetStore::Create(const RenderTargetDesc &desc,
	GLTextureStore &textures) {
	if (desc.width <= 0 || desc.height <= 0) {
		Log::Error("GLRenderTargetStore::Create: invalid size");
		return {};
	}

	const TextureHandle color = textures.Create(desc.width, desc.height, nullptr, desc.filter);
	if (!color.IsValid()) {
		return {};
	}

	u32 fbo = 0;
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
		textures.GlId(color), 0);
	u32 uintTexture = 0;
	if (desc.unsignedIntegerAttachment) {
		glGenTextures(1, &uintTexture);
		glBindTexture(GL_TEXTURE_2D, uintTexture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_R32UI, desc.width, desc.height, 0, GL_RED_INTEGER,
			GL_UNSIGNED_INT, nullptr);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, uintTexture, 0);
	}
	const GLenum drawBuffers[] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
	glDrawBuffers(desc.unsignedIntegerAttachment ? 2 : 1, drawBuffers);

	const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	if (status != GL_FRAMEBUFFER_COMPLETE) {
		Log::Error("GLRenderTargetStore::Create: framebuffer incomplete ({})",
			static_cast<int>(status));
		textures.Destroy(color);
		if (uintTexture != 0) {
			glDeleteTextures(1, &uintTexture);
		}
		glDeleteFramebuffers(1, &fbo);
		return {};
	}

	u32 slotIndex = 0;
	bool found = false;
	for (; slotIndex < _slots.size(); ++slotIndex) {
		if (!_slots[slotIndex].occupied) {
			found = true;
			break;
		}
	}
	if (!found) {
		slotIndex = static_cast<u32>(_slots.size());
		_slots.push_back({});
	}

	Slot &slot = _slots[slotIndex];
	slot.glFbo = fbo;
	slot.glUIntTexture = uintTexture;
	slot.colorTexture = color;
	slot.width = desc.width;
	slot.height = desc.height;
	slot.occupied = true;

	return RenderTargetHandle{slotIndex + 1, slot.version};
}

void GLRenderTargetStore::Destroy(RenderTargetHandle handle, GLTextureStore &textures) {
	Slot *slot = resolve(handle);
	if (!slot) {
		return;
	}

	if (slot->glFbo != 0) {
		glDeleteFramebuffers(1, &slot->glFbo);
		slot->glFbo = 0;
	}
	if (slot->glUIntTexture != 0) {
		glDeleteTextures(1, &slot->glUIntTexture);
		slot->glUIntTexture = 0;
	}
	if (slot->colorTexture.IsValid()) {
		textures.Destroy(slot->colorTexture);
		slot->colorTexture = {};
	}
	slot->width = 0;
	slot->height = 0;
	slot->occupied = false;
	++slot->version;
}

void GLRenderTargetStore::Bind(RenderTargetHandle handle) const {
	if (!handle.IsValid()) {
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		return;
	}

	const Slot *slot = resolve(handle);
	if (!slot) {
		Log::Warn("GLRenderTargetStore::Bind: invalid or stale handle");
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		return;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, slot->glFbo);
	glViewport(0, 0, slot->width, slot->height);
}

void GLRenderTargetStore::DestroyAll(GLTextureStore &textures) {
	for (Slot &slot : _slots) {
		if (!slot.occupied) {
			continue;
		}
		if (slot.glFbo != 0) {
			glDeleteFramebuffers(1, &slot.glFbo);
			slot.glFbo = 0;
		}
		if (slot.glUIntTexture != 0) {
			glDeleteTextures(1, &slot.glUIntTexture);
			slot.glUIntTexture = 0;
		}
		if (slot.colorTexture.IsValid()) {
			textures.Destroy(slot.colorTexture);
			slot.colorTexture = {};
		}
		slot.width = 0;
		slot.height = 0;
		slot.occupied = false;
		++slot.version;
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

TextureHandle GLRenderTargetStore::ColorTexture(RenderTargetHandle handle) const {
	const Slot *slot = resolve(handle);
	return slot ? slot->colorTexture : TextureHandle{};
}

void GLRenderTargetStore::ClearUInt(RenderTargetHandle handle, u32 attachment, u32 value) const {
	const Slot *slot = resolve(handle);
	if (!slot || attachment != 1 || slot->glUIntTexture == 0) {
		return;
	}
	GLint previous = 0;
	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &previous);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, slot->glFbo);
	glClearBufferuiv(GL_COLOR, static_cast<GLint>(attachment), &value);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, static_cast<GLuint>(previous));
}

u32 GLRenderTargetStore::ReadUInt(RenderTargetHandle handle, u32 attachment, i32 x, i32 y) const {
	const Slot *slot = resolve(handle);
	if (!slot || attachment != 1 || slot->glUIntTexture == 0 || x < 0 || y < 0 ||
		x >= slot->width || y >= slot->height) {
		return 0;
	}

	GLint previous = 0;
	glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &previous);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, slot->glFbo);
	glReadBuffer(GL_COLOR_ATTACHMENT0 + attachment);
	// RGBA_INTEGER: the one integer read format WebGL2 guarantees.
	u32 rgba[4] = {};
	glReadPixels(x, y, 1, 1, GL_RGBA_INTEGER, GL_UNSIGNED_INT, rgba);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, static_cast<GLuint>(previous));
	return rgba[0];
}

std::vector<u32> GLRenderTargetStore::ReadUIntRect(RenderTargetHandle handle, u32 attachment, i32 x,
	i32 y, i32 width, i32 height) const {
	const Slot *slot = resolve(handle);
	if (!slot || attachment != 1 || !slot->glUIntTexture || x < 0 || y < 0 || width <= 0 ||
		height <= 0 || width > slot->width || height > slot->height || x > slot->width - width ||
		y > slot->height - height) {
		return {};
	}
	const std::size_t count = static_cast<std::size_t>(width) * height;
	std::vector<u32> rgba(count * 4);
	GLint previous = 0, pack = 0, rowLength = 0, skipRows = 0, skipPixels = 0, buffer = 0;
	glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &previous);
	glGetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING, &buffer);
	glGetIntegerv(GL_PACK_ALIGNMENT, &pack);
	glGetIntegerv(GL_PACK_ROW_LENGTH, &rowLength);
	glGetIntegerv(GL_PACK_SKIP_ROWS, &skipRows);
	glGetIntegerv(GL_PACK_SKIP_PIXELS, &skipPixels);
	glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
	glPixelStorei(GL_PACK_ALIGNMENT, 4);
	glPixelStorei(GL_PACK_ROW_LENGTH, 0);
	glPixelStorei(GL_PACK_SKIP_ROWS, 0);
	glPixelStorei(GL_PACK_SKIP_PIXELS, 0);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, slot->glFbo);
	GLint readBuffer = 0;
	glGetIntegerv(GL_READ_BUFFER, &readBuffer);
	glReadBuffer(GL_COLOR_ATTACHMENT0 + attachment);
	glReadPixels(x, y, width, height, GL_RGBA_INTEGER, GL_UNSIGNED_INT, rgba.data());
	glReadBuffer(static_cast<GLenum>(readBuffer));
	glBindFramebuffer(GL_READ_FRAMEBUFFER, static_cast<GLuint>(previous));
	glBindBuffer(GL_PIXEL_PACK_BUFFER, static_cast<GLuint>(buffer));
	glPixelStorei(GL_PACK_ALIGNMENT, pack);
	glPixelStorei(GL_PACK_ROW_LENGTH, rowLength);
	glPixelStorei(GL_PACK_SKIP_ROWS, skipRows);
	glPixelStorei(GL_PACK_SKIP_PIXELS, skipPixels);
	std::vector<u32> result(count);
	for (std::size_t i = 0; i < count; ++i) {
		result[i] = rgba[i * 4];
	}
	return result;
}

i32 GLRenderTargetStore::Width(RenderTargetHandle handle) const {
	const Slot *slot = resolve(handle);
	return slot ? slot->width : 0;
}

i32 GLRenderTargetStore::Height(RenderTargetHandle handle) const {
	const Slot *slot = resolve(handle);
	return slot ? slot->height : 0;
}

GLRenderTargetStore::Slot *GLRenderTargetStore::resolve(RenderTargetHandle handle) {
	return const_cast<Slot *>(static_cast<const GLRenderTargetStore *>(this)->resolve(handle));
}

const GLRenderTargetStore::Slot *GLRenderTargetStore::resolve(RenderTargetHandle handle) const {
	if (!handle.IsValid()) {
		return nullptr;
	}
	const u32 index = handle.id - 1;
	if (index >= _slots.size()) {
		return nullptr;
	}
	const Slot &slot = _slots[index];
	if (!slot.occupied || slot.version != handle.version) {
		return nullptr;
	}
	return &slot;
}

} // namespace sowa
