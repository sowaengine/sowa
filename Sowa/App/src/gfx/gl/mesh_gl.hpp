#ifndef SW_MESH_GL_HPP_
#define SW_MESH_GL_HPP_

#include <cstdint>
#include <unordered_map>

#include "buffer_gl.hpp"
#include "vertex_array_gl.hpp"

namespace sowa {
namespace gfx {
class GLMesh {
  public:
    GLMesh() = default;
    ~GLMesh();

	void SetModelData(const std::vector<float>& data);
	void SetIndexData(const std::vector<uint32_t>& data);
	void ResetAttributes();
	void SetAttribute(uint32_t slot, GLAttributeType type);

	void New();
    void Delete();

	void Draw();

  private:
	GLVertexArray m_vao;
	GLBuffer m_vbo;
	GLBuffer m_ebo;

	uint32_t m_indexCount = 0;
	uint32_t m_vboData_size = 0;
};
} // namespace gfx
} // namespace sowa

#endif // SW_MESH_GL_HPP_