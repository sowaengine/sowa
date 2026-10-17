#include "render/gl/gl_buffer.hpp"

#include "render/gl/gl.hpp"

#include "core/log.hpp"

namespace sowa {
namespace {

GLenum toGlTarget(BufferType type) {
	return type == BufferType::Index ? GL_ELEMENT_ARRAY_BUFFER : GL_ARRAY_BUFFER;
}

GLenum toGlUsage(BufferUsage usage) {
	return usage == BufferUsage::Dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW;
}

} // namespace

BufferHandle GLBufferStore::Create(BufferType type, BufferUsage usage, u32 size, const void *data) {
	u32 glBuffer = 0;
	glGenBuffers(1, &glBuffer);

	const GLenum target = toGlTarget(type);
	glBindBuffer(target, glBuffer);
	glBufferData(target, static_cast<GLsizeiptr>(size), data, toGlUsage(usage));
	glBindBuffer(target, 0);

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
	slot.glBuffer = glBuffer;
	slot.type = type;
	slot.size = size;
	slot.occupied = true;

	return BufferHandle{slotIndex + 1, slot.version};
}

void GLBufferStore::Destroy(BufferHandle handle) {
	Slot *slot = resolve(handle);
	if (!slot) {
		return;
	}

	if (slot->glBuffer != 0) {
		glDeleteBuffers(1, &slot->glBuffer);
		slot->glBuffer = 0;
	}
	slot->size = 0;
	slot->occupied = false;
	++slot->version;
}

void GLBufferStore::Update(BufferHandle handle, const void *data, u32 size, u32 offset) {
	Slot *slot = resolve(handle);
	if (!slot) {
		Log::Warn("GLBufferStore::Update: invalid or stale handle");
		return;
	}
	if (offset + size > slot->size) {
		Log::Error("GLBufferStore::Update: write exceeds buffer size "
				   "(offset={}, size={}, capacity={})",
			offset, size, slot->size);
		return;
	}

	const GLenum target = toGlTarget(slot->type);
	glBindBuffer(target, slot->glBuffer);
	glBufferSubData(target, static_cast<GLintptr>(offset), static_cast<GLsizeiptr>(size), data);
	glBindBuffer(target, 0);
}

void GLBufferStore::DestroyAll() {
	for (Slot &slot : _slots) {
		if (slot.occupied && slot.glBuffer != 0) {
			glDeleteBuffers(1, &slot.glBuffer);
		}
		slot.glBuffer = 0;
		slot.size = 0;
		slot.occupied = false;
		++slot.version;
	}
}

u32 GLBufferStore::GlId(BufferHandle handle) const {
	const Slot *slot = resolve(handle);
	return slot ? slot->glBuffer : 0;
}

BufferType GLBufferStore::Type(BufferHandle handle) const {
	const Slot *slot = resolve(handle);
	return slot ? slot->type : BufferType::Vertex;
}

GLBufferStore::Slot *GLBufferStore::resolve(BufferHandle handle) {
	return const_cast<Slot *>(static_cast<const GLBufferStore *>(this)->resolve(handle));
}

const GLBufferStore::Slot *GLBufferStore::resolve(BufferHandle handle) const {
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
