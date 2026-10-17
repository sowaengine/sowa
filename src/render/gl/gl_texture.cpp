#include "render/gl/gl_texture.hpp"

#include "render/gl/gl.hpp"

#include "core/log.hpp"

namespace sowa {

TextureHandle GLTextureStore::Create(i32 width, i32 height, const void *rgbaPixels,
	TextureFilter filter) {
	if (width <= 0 || height <= 0) {
		Log::Error("GLTextureStore::Create: invalid size");
		return {};
	}

	u32 glTexture = 0;
	glGenTextures(1, &glTexture);
	glBindTexture(GL_TEXTURE_2D, glTexture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	const GLint glFilter = filter == TextureFilter::Nearest ? GL_NEAREST : GL_LINEAR;
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, glFilter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, glFilter);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
		rgbaPixels);
	glBindTexture(GL_TEXTURE_2D, 0);

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
	slot.glTexture = glTexture;
	slot.width = width;
	slot.height = height;
	slot.occupied = true;

	return TextureHandle{slotIndex + 1, slot.version};
}

void GLTextureStore::Destroy(TextureHandle handle) {
	Slot *slot = resolve(handle);
	if (!slot) {
		return;
	}

	if (slot->glTexture != 0) {
		glDeleteTextures(1, &slot->glTexture);
		slot->glTexture = 0;
	}
	slot->width = 0;
	slot->height = 0;
	slot->occupied = false;
	++slot->version;
}

void GLTextureStore::Bind(TextureHandle handle, u32 slot) const {
	const Slot *tex = resolve(handle);
	if (!tex) {
		Log::Warn("GLTextureStore::Bind: invalid or stale handle");
		glActiveTexture(GL_TEXTURE0 + slot);
		glBindTexture(GL_TEXTURE_2D, 0);
		return;
	}

	glActiveTexture(GL_TEXTURE0 + static_cast<GLenum>(slot));
	glBindTexture(GL_TEXTURE_2D, tex->glTexture);
}

void GLTextureStore::DestroyAll() {
	for (Slot &slot : _slots) {
		if (slot.occupied && slot.glTexture != 0) {
			glDeleteTextures(1, &slot.glTexture);
		}
		slot.glTexture = 0;
		slot.width = 0;
		slot.height = 0;
		slot.occupied = false;
		++slot.version;
	}
}

u32 GLTextureStore::GlId(TextureHandle handle) const {
	const Slot *slot = resolve(handle);
	return slot ? slot->glTexture : 0;
}

i32 GLTextureStore::Width(TextureHandle handle) const {
	const Slot *slot = resolve(handle);
	return slot ? slot->width : 0;
}

i32 GLTextureStore::Height(TextureHandle handle) const {
	const Slot *slot = resolve(handle);
	return slot ? slot->height : 0;
}

GLTextureStore::Slot *GLTextureStore::resolve(TextureHandle handle) {
	return const_cast<Slot *>(static_cast<const GLTextureStore *>(this)->resolve(handle));
}

const GLTextureStore::Slot *GLTextureStore::resolve(TextureHandle handle) const {
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
