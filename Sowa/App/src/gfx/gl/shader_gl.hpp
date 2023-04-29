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

  private:
	std::string m_vertexSource = "";
	std::string m_fragmentSource = "";

	uint32_t m_id = 0;
};
} // namespace gfx
} // namespace sowa

#endif // SW_SHADER_GL_HPP_