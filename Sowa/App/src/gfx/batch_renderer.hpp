#ifndef SW_BATCH_RENDERER_HPP_
#define SW_BATCH_RENDERER_HPP_

#include "gfx/shader.hpp"
#include "glm/glm.hpp"

namespace sowa {
namespace gfx {
struct BatchVertex;

class IBatchRenderer {
  public:
	virtual void Init(const std::string &vsource, const std::string &fsource) = 0;

	virtual void Reset() = 0;
	virtual void PushQuad(BatchVertex vertices[4]) = 0;
	virtual void PushLine(glm::vec2 p1, glm::vec2 p2, float thickness, glm::vec4 color = glm::vec4{1.f, 1.f, 1.f, 1.f}, uint32_t id = 0) = 0;
	virtual void End() = 0;

	virtual IShader &Shader() = 0;

  private:
};
} // namespace gfx
} // namespace sowa

#endif // SW_BATCH_RENDERER_HPP_