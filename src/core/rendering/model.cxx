#include "model.hxx"

#include <iostream>

Model::Model() {
}

Model::~Model() {
	Delete();
}

void Model::SetModelData(const std::vector<float> &data) {
	m_vboSize = data.size();

	m_vao.Bind();
	m_vbo.Bind();
	m_vbo.BufferData(data.data(), data.size() * sizeof(float), gfx::BufferUsage::StaticDraw);
	m_vao.Unbind();
}
void Model::SetIndexData(const std::vector<uint32_t> &data) {
	m_indexCount = data.size();

	m_vao.Bind();
	m_ebo.Bind();
	m_ebo.BufferData(data.data(), data.size() * sizeof(uint32_t), gfx::BufferUsage::StaticDraw);
	m_vao.Unbind();
}

void Model::ResetAttributes() {
	m_vao.Bind();
	m_vao.ResetAttributes();
	m_vao.Unbind();
}
void Model::SetAttribute(uint32_t slot, gfx::attribute_t type) {
	m_vao.SetAttribute(slot, type);
}
void Model::UploadAttributes() {
	m_vao.Bind();
	m_vbo.Bind();
	if (m_indexCount > 0) {
		m_ebo.Bind();
	}
	m_vao.UploadAttributes();
	m_vao.Unbind();
}

void Model::New() {
	Delete();

	m_vao.New();
	m_vbo.New(gfx::BufferType::VertexBuffer);
	m_ebo.New(gfx::BufferType::IndexBuffer);
}
void Model::Delete() {
	m_vao.Delete();
	m_vbo.Delete();
	m_ebo.Delete();
}

void Model::Draw() {
	m_vao.Bind();
	if (m_indexCount > 0) {
		glDrawElements(GL_TRIANGLES, m_indexCount, GL_UNSIGNED_INT, nullptr);
	} else if (m_vao.SizeInBytes() > 0) {
		glDrawArrays(GL_TRIANGLES, 0, m_vboSize);
	}
	m_vao.Unbind();
}