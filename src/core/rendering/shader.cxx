#include "shader.hxx"

#include "gl.hxx"

#include "servers/file_server.hxx"
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

ErrorCode Shader::Load(const char *vertexPath, const char *fragmentPath) {
	ErrorCode err;
	std::string buf;
	Result<file_buffer *> res = FileServer::get().load_file(vertexPath, ReadWriteFlags_AsText);
	if (!res.ok()) {
		SetVertexSource("");
		SetFragmentSource("");
		return res.error();
	}
	buf = std::string((char *)res.get()->data(), res.get()->size());
	SetVertexSource(buf);

	res = FileServer::get().load_file(fragmentPath, ReadWriteFlags_AsText);
	if (!res.ok()) {
		SetVertexSource("");
		SetFragmentSource("");
		return res.error();
	}
	buf = std::string((char *)res.get()->data(), res.get()->size());
	SetFragmentSource(buf);

	return Build();
}

void Shader::SetVertexSource(const std::string &src) {
	m_vertexSource = src;
}

void Shader::SetFragmentSource(const std::string &src) {
	m_fragmentSource = src;
}

ErrorCode Shader::Build() {
	Delete();

	uint32_t vertexShader = glCreateShader(GL_VERTEX_SHADER);
	const char *vertexSrc = m_vertexSource.c_str();
	glShaderSource(vertexShader, 1, &vertexSrc, nullptr);
	glCompileShader(vertexShader);
	if (!HandleCompileError(vertexShader, "vertex")) {
		return ERR_FAILED;
	}

	uint32_t fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	const char *fragmentSrc = m_fragmentSource.c_str();
	glShaderSource(fragmentShader, 1, &fragmentSrc, nullptr);
	glCompileShader(fragmentShader);
	if (!HandleCompileError(fragmentShader, "fragment")) {
		return ERR_FAILED;
	}

	m_id = glCreateProgram();
	glAttachShader(m_id, vertexShader);
	glAttachShader(m_id, fragmentShader);
	glLinkProgram(m_id);
	if (!HandleLinkError(m_id)) {
		return ERR_FAILED;
	}

	return OK;
}

ErrorCode Shader::New(const std::string &vertexSource, const std::string &fragmentSource) {
	SetVertexSource(vertexSource);
	SetFragmentSource(fragmentSource);
	return Build();
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

void Shader::Uniformiv(const char *name, const std::vector<int> &value) {
	Bind();
	int loc = glGetUniformLocation(m_id, name);
	if (loc < 0) {
		return;
	}

	glUniform1iv(loc, value.size(), value.data());
}

void Shader::UniformMat4(const char *name, const glm::mat4 &value) {
	Bind();
	int loc = glGetUniformLocation(m_id, name);
	if (loc < 0) {
		return;
	}

	glUniformMatrix4fv(loc, 1, GL_FALSE, &value[0][0]);
}