#ifndef SW_SHADER_HXX
#define SW_SHADER_HXX
#pragma once

#include "core/error/error.hxx"
#include "glm/glm.hpp"
#include <string>
#include <vector>

class Shader {
  public:
	Shader() = default;
	~Shader();

	ErrorCode Load(const char *vertexPath, const char *fragmentPath);

	void SetVertexSource(const std::string &src);
	void SetFragmentSource(const std::string &src);
	ErrorCode Build();

	ErrorCode New(const std::string &vertexSource, const std::string &fragmentSource);
	void Delete();

	void Bind();
	void Unbind();

	void Uniformiv(const char *name, const std::vector<int> &value);
	void UniformMat4(const char *name, const glm::mat4 &value);

	inline uint32_t ID() const { return m_id; }

  private:
	uint32_t m_id = 0;

	std::string m_vertexSource = "";
	std::string m_fragmentSource = "";
};

#endif // SW_SHADER_HXX