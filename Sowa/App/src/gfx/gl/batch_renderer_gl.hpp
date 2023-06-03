#ifndef SW_BATCH_RENDERER_GL_HPP_
#define SW_BATCH_RENDERER_GL_HPP_

#include <map>
#include <vector>

#include "../batch_renderer.hpp"
#include "../graphics.hpp"
#include "./buffer_gl.hpp"
#include "./shader_gl.hpp"
#include "./vertex_array_gl.hpp"

namespace sowa {
namespace gfx {
class BatchRendererGL : public IBatchRenderer {
  public:
	void Init(const std::string &vsource, const std::string &fsource) override;

    void Reset() override;
	void PushQuad(BatchVertex vertices[4]) override;
	void PushLine(glm::vec2 p1, glm::vec2 p2, float thickness, glm::vec4 color = glm::vec4{1.f, 1.f, 1.f, 1.f}, uint32_t id = 0) override;
	void End() override;

    inline IShader& Shader() { return m_shader; }

  private:
	GLShader m_shader;
	GLBuffer m_buffer;
	GLVertexArray m_array;
	std::vector<BatchVertex> m_vertices;

	// textures[textureId] = slot;
	std::map<uint32_t, uint32_t> m_textures;
	uint32_t m_textureCounter = 0;
};
} // namespace gfx
} // namespace sowa

#endif // SW_BATCH_RENDERER_GL_HPP_