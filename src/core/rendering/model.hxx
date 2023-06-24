#ifndef SW_MODEL_HXX
#define SW_MODEL_HXX
#pragma once

#include "gfx/buffer.hxx"
#include "gfx/vertex_array.hxx"
#include <cstdint>
#include <vector>

class Model {
  public:
	Model();
	~Model();

	void SetModelData(const std::vector<float> &data);
	void SetIndexData(const std::vector<uint32_t> &data);

	void ResetAttributes();
	void SetAttribute(uint32_t slot, gfx::attribute_t type);
	void UploadAttributes();

	void New();
	void Delete();

	void Draw();

  private:
	gfx::VertexArray m_vao;
	gfx::Buffer m_vbo;
	gfx::Buffer m_ebo;

	uint32_t m_indexCount = 0;
	uint32_t m_vboSize = 0;
};

#endif // SW_MODEL_HXX