#include "shader.hxx"

#ifdef SW_WEB
#include <GLES3/gl3.h>
#include <GLFW/glfw3.h>
#include <emscripten.h>
#else
#include <glad/glad.h>
//
#include <GLFW/glfw3.h>
#endif

#include <iostream>

static bool HandleCompileError(uint32_t id, const char *shaderType) {
	int success;
	char infoLog[512];
	glGetShaderiv(id, GL_COMPILE_STATUS, &success);

	if (!success) {
		glGetShaderInfoLog(id, 512, NULL, infoLog);
		std::cout << shaderType << " shader error:"
				  << infoLog << std::endl;

		return false;
	}
	return true;
}

static bool HandleLinkError(uint32_t id) {
	int success;
	char infoLog[512];
	glGetProgramiv(id, GL_LINK_STATUS, &success);

	if (!success) {
		glGetProgramInfoLog(id, 512, NULL, infoLog);
		std::cout << "Shader program compilation failed"
				  << infoLog << std::endl;
		return false;
	}
	return true;
}

Shader::~Shader() {
	Delete();
}

void Shader::SetVertexSource(const std::string &src) {
	m_vertexSource = src;
}

void Shader::SetFragmentSource(const std::string &src) {
	m_fragmentSource = src;
}

void Shader::Build() {
	Delete();

	uint32_t vertexShader = glCreateShader(GL_VERTEX_SHADER);
	const char *vertexSrc = m_vertexSource.c_str();
	glShaderSource(vertexShader, 1, &vertexSrc, nullptr);
	glCompileShader(vertexShader);
	if (!HandleCompileError(vertexShader, "vertex")) {
		return;
	}

	uint32_t fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	const char *fragmentSrc = m_fragmentSource.c_str();
	glShaderSource(fragmentShader, 1, &fragmentSrc, nullptr);
	glCompileShader(fragmentShader);
	if (!HandleCompileError(fragmentShader, "fragment")) {
		return;
	}

	m_id = glCreateProgram();
	glAttachShader(m_id, vertexShader);
	glAttachShader(m_id, fragmentShader);
	glLinkProgram(m_id);
	if (!HandleLinkError(m_id)) {
		return;
	}
}

void Shader::New(const std::string &vertexSource, const std::string &fragmentSource) {
	SetVertexSource(vertexSource);
	SetFragmentSource(fragmentSource);
	Build();
}

void Shader::Delete() {
	if (m_id != 0) {
		glDeleteProgram(m_id);
	}

	m_id = 0;
}

void Shader::Bind() {
	glUseProgram(m_id);
}

void Shader::Unbind() {
	glUseProgram(0);
}