#include "shader_gl.hpp"

#include "glad/glad.h"

#include "debug.hpp"
#include "gfx/gl/glfuncs.hpp"

namespace sowa {
namespace gfx {

GLShader::~GLShader() {
	Delete();
}

void GLShader::SetVertexSource(const std::string &src) {
	if (m_id != 0)
		return;

	m_vertexSource = src;
}
void GLShader::SetFragmentSource(const std::string &src) {
	if (m_id != 0)
		return;

	m_fragmentSource = src;
}

void GLShader::Build() {
	if (m_id != 0) {
		Delete();
	}

	uint32_t vertexShader = GL().createShader(GLShaderType::Vertex);
	GL().shaderSource(vertexShader, m_vertexSource);
	GL().compileShader(vertexShader, GLShaderType::Vertex);

	uint32_t fragmentShader = GL().createShader(GLShaderType::Fragment);
	GL().shaderSource(fragmentShader, m_fragmentSource);
	GL().compileShader(fragmentShader, GLShaderType::Fragment);

	m_id = GL().createProgram();
	GL().attachShader(m_id, vertexShader);
	GL().attachShader(m_id, fragmentShader);
	GL().linkProgram(m_id);
}

void GLShader::New(const std::string &vsource, const std::string &fsource) {
	SetVertexSource(vsource);
	SetFragmentSource(fsource);
	Build();
}

void GLShader::Delete() {
	if (m_id != 0) {
		GL().deleteProgram(m_id);
	}
}

void GLShader::Bind() {
	GL().useProgram(m_id);
}
void GLShader::Unbind() {
	GL().useProgram(0);
}

void GLShader::UniformTexture(const std::string& name, uint32_t textureID, int slot) {
	Bind();
	int loc = GL().getUniformLocation(m_id, name);
	if(loc < 0) {
		Debug::Error("Failed to get uniform location {}", name);
	}

	glActiveTexture(GL_TEXTURE0);
	// glActiveTexture(GL_TEXTURE0 + slot);
	glBindTexture(GL_TEXTURE_2D, textureID);
	glUniform1i(loc, slot);

	// GL().activeTexture(slot);
	// GL().bindTexture(GLTextureType::Texture2D, textureID);
	// GL().uniform1i(loc, slot);
}

void GLShader::UniformMat4(const std::string& name, const mat4& value) {
	Bind();
	int loc = GL().getUniformLocation(m_id, name);
	if(loc < 0) {
		Debug::Error("Failed to get uniform location {}", name);
	}
	
	glUniformMatrix4fv(loc, 1, GL_FALSE, &value[0][0]);
}

void GLShader::UniformVec2(const std::string& name, const glm::vec2& value) {
	Bind();
	int loc = GL().getUniformLocation(m_id, name);
	if(loc < 0) {
		Debug::Error("Failed to get uniform location {}", name);
	}

	glUniform2f(loc, value.x, value.y);
}

void GLShader::UniformVec4(const std::string& name, const glm::vec4& value) {
	Bind();
	int loc = GL().getUniformLocation(m_id, name);
	if(loc < 0) {
		Debug::Error("Failed to get uniform location {}", name);
	}

	glUniform4f(loc, value.x, value.y, value.z, value.w);
}

void GLShader::UniformIntVector(const std::string& name, const std::vector<int>& value) {
	Bind();
	int loc = GL().getUniformLocation(m_id, name);
	if(loc < 0) {
		Debug::Error("Failed to get uniform location {}", name);
	}

	glUniform1iv(loc, value.size(), value.data());
}

} // namespace gfx
} // namespace sowa
