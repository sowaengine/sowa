#ifndef SW_SHADER_GL_HPP_
#define SW_SHADER_GL_HPP_

#include "../shader.hpp"
#include <string>

namespace sowa {
namespace gfx {

enum class GLShaderType {
	Vertex = 0,
	Fragment,
};

class GLShader : public IShader {
  public:
	GLShader() = default;
	~GLShader();

	void SetVertexSource(const std::string &src);
	void SetFragmentSource(const std::string &src);
	void Build();


	void New(const std::string &vsource, const std::string& fsource) override;
	void Delete() override;

	void Bind() override;
	void Unbind() override;

	const uint32_t ID() override { return m_id; }

	void UniformTexture(const std::string& name, uint32_t textureID, int slot) override;
	void UniformMat4(const std::string& name, const mat4& value) override;
	void UniformVec2(const std::string& name, const glm::vec2& value) override;
	void UniformVec4(const std::string& name, const glm::vec4& value) override;
	void UniformIntVector(const std::string& name, const std::vector<int>& value) override;

  private:
	std::string m_vertexSource = "";
	std::string m_fragmentSource = "";

	uint32_t m_id = 0;
};
} // namespace gfx
} // namespace sowa

#endif // SW_SHADER_GL_HPP_