#ifndef SW_SHADER_HXX
#define SW_SHADER_HXX
#pragma once

#include <string>

class Shader {
  public:
	Shader() = default;
	~Shader();

	void SetVertexSource(const std::string &src);
	void SetFragmentSource(const std::string &src);
	void Build();

	void New(const std::string &vertexSource, const std::string &fragmentSource);
	void Delete();

	void Bind();
	void Unbind();

	inline uint32_t ID() const { return m_id; }

  private:
	uint32_t m_id = 0;

	std::string m_vertexSource = "";
	std::string m_fragmentSource = "";
};

#endif // SW_SHADER_HXX