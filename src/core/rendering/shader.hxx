#ifndef SW_SHADER_HXX
#define SW_SHADER_HXX
#pragma once

#include "core/error/error.hxx"
#include <string>

class Shader {
  public:
	Shader() = default;
	~Shader();

	Error Load(const char *vertexPath, const char *fragmentPath);

	void SetVertexSource(const std::string &src);
	void SetFragmentSource(const std::string &src);
	Error Build();

	Error New(const std::string &vertexSource, const std::string &fragmentSource);
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