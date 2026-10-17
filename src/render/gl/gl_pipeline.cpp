#include "render/gl/gl_pipeline.hpp"

#include <cstdint>

#include "render/gl/gl.hpp"
#include "render/gl/gl_shader.hpp"

#include "core/log.hpp"

namespace sowa {
namespace {

GLenum toGlAttrType(VertexAttrType type) {
	return type == VertexAttrType::UInt32 ? GL_UNSIGNED_INT : GL_FLOAT;
}

} // namespace

PipelineHandle GLPipelineStore::Create(const PipelineDesc &desc, GLShaderStore &shaders) {
	const u32 program = shaders.ProgramId(desc.shader);
	if (program == 0) {
		Log::Error("GLPipelineStore::Create: invalid shader handle");
		return {};
	}
	if (desc.layout.attributes.empty()) {
		Log::Error("GLPipelineStore::Create: empty vertex layout");
		return {};
	}

	u32 vao = 0;
	glGenVertexArrays(1, &vao);

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
	slot.glVao = vao;
	slot.glProgram = program;
	slot.layout = desc.layout;
	if (slot.layout.stride == 0) {
		slot.layout.stride = VertexLayoutStride(slot.layout);
	}
	slot.alphaBlend = desc.alphaBlend;
	slot.additiveBlend = desc.additiveBlend;
	slot.occupied = true;

	return PipelineHandle{slotIndex + 1, slot.version};
}

void GLPipelineStore::Destroy(PipelineHandle handle) {
	Slot *slot = resolve(handle);
	if (!slot) {
		return;
	}

	if (slot->glVao != 0) {
		glDeleteVertexArrays(1, &slot->glVao);
		slot->glVao = 0;
	}
	slot->glProgram = 0;
	slot->layout = {};
	slot->occupied = false;
	++slot->version;
}

void GLPipelineStore::Bind(PipelineHandle handle) const {
	if (!handle.IsValid()) {
		glUseProgram(0);
		glBindVertexArray(0);
		glDisable(GL_BLEND);
		return;
	}

	const Slot *slot = resolve(handle);
	if (!slot) {
		Log::Warn("GLPipelineStore::Bind: invalid or stale handle");
		glUseProgram(0);
		glBindVertexArray(0);
		return;
	}

	glUseProgram(slot->glProgram);
	glBindVertexArray(slot->glVao);

	if (slot->alphaBlend) {
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, slot->additiveBlend ? GL_ONE : GL_ONE_MINUS_SRC_ALPHA);
	} else {
		glDisable(GL_BLEND);
	}
}

void GLPipelineStore::ApplyVertexLayout(PipelineHandle handle) const {
	const Slot *slot = resolve(handle);
	if (!slot) {
		return;
	}

	const u32 stride = slot->layout.stride;
	u32 offset = 0;
	for (const VertexAttribute &attr : slot->layout.attributes) {
		glEnableVertexAttribArray(attr.location);
		if (attr.type == VertexAttrType::UInt32) {
			glVertexAttribIPointer(attr.location,
				static_cast<GLint>(VertexAttrTypeComponents(attr.type)), toGlAttrType(attr.type),
				static_cast<GLsizei>(stride),
				reinterpret_cast<const void *>(static_cast<uintptr_t>(offset)));
		} else {
			glVertexAttribPointer(attr.location,
				static_cast<GLint>(VertexAttrTypeComponents(attr.type)), toGlAttrType(attr.type),
				GL_FALSE, static_cast<GLsizei>(stride),
				reinterpret_cast<const void *>(static_cast<uintptr_t>(offset)));
		}
		offset += VertexAttrTypeSize(attr.type);
	}
}

void GLPipelineStore::DestroyAll() {
	for (Slot &slot : _slots) {
		if (slot.occupied && slot.glVao != 0) {
			glDeleteVertexArrays(1, &slot.glVao);
		}
		slot.glVao = 0;
		slot.glProgram = 0;
		slot.layout = {};
		slot.occupied = false;
		++slot.version;
	}
	glUseProgram(0);
	glBindVertexArray(0);
}

GLPipelineStore::Slot *GLPipelineStore::resolve(PipelineHandle handle) {
	return const_cast<Slot *>(static_cast<const GLPipelineStore *>(this)->resolve(handle));
}

const GLPipelineStore::Slot *GLPipelineStore::resolve(PipelineHandle handle) const {
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
