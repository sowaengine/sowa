#include "mesh_gl.hpp"

#include <vector>

#include "glad/glad.h"
#include "tiny_obj_loader.h"

#include "debug.hpp"
#include "gfx/gl/glfuncs.hpp"

namespace sowa {
namespace gfx {
GLMesh::~GLMesh() {
}

void GLMesh::SetModelData(const std::vector<float> &data) {
	m_vboData_size = data.size();

	m_vao.Bind();
	m_vbo.Bind();
	m_vbo.BufferData(data.data(), data.size() * sizeof(float), BufferUsage::StaticDraw);
	m_vao.Unbind();
}
void GLMesh::SetIndexData(const std::vector<uint32_t> &data) {
	m_indexCount = data.size();

	m_vao.Bind();
	m_ebo.Bind();
	m_ebo.BufferData(data.data(), data.size() * sizeof(uint32_t), BufferUsage::StaticDraw);
	m_vao.Unbind();
}
void GLMesh::ResetAttributes() {
	m_vao.Bind();
	m_vao.ResetAttributes();
	m_vao.Unbind();
}
void GLMesh::SetAttribute(uint32_t slot, GLAttributeType type) {
	m_vao.SetAttribute(slot, type);
}

void GLMesh::UploadAttributes() {
	m_vao.Bind();
	m_vbo.Bind();
	if (m_indexCount > 0) {
		m_ebo.Bind();
	}
	m_vao.UploadAttributes();
	m_vao.Unbind();
}

void GLMesh::New() {
	m_vao.New();
	m_vbo.New(BufferType::VertexBuffer);
	m_ebo.New(BufferType::IndexBuffer);
}
void GLMesh::Delete() {
	m_vao.Delete();
	m_vbo.Delete();
	m_ebo.Delete();
}

void GLMesh::Draw() {
	m_vao.Bind();
	if (m_indexCount > 0) {
		GL().drawElements(GLDrawMode::Triangles, m_indexCount);
	} else if (m_vao.m_attributeSizeInBytes > 0) {
		GL().drawArrays(GLDrawMode::Triangles, 0, m_vboData_size / m_vao.m_attributeSizeInBytes);
	}
	m_vao.Unbind();
}
} // namespace gfx
} // namespace sowa
