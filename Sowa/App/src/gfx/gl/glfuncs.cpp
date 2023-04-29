#include "gfx/gl/glfuncs.hpp"

#include "glad/glad.h"

#include "debug.hpp"

namespace sowa {
namespace gfx {

static GLenum ToGL(BufferType type) {
	return type == BufferType::VertexBuffer	 ? GL_ARRAY_BUFFER
		   : type == BufferType::IndexBuffer ? GL_ELEMENT_ARRAY_BUFFER
											 : GL_NONE;
}
static GLenum ToGL(BufferUsage usage) {
	return usage == BufferUsage::StaticDraw	   ? GL_STATIC_DRAW
		   : usage == BufferUsage::DynamicDraw ? GL_DYNAMIC_DRAW
											   : GL_NONE;
}

static GLenum ToGL(GLAttributeType type) {
	return type == GLAttributeType::Float  ? GL_FLOAT
		   : type == GLAttributeType::Vec2 ? GL_FLOAT
		   : type == GLAttributeType::Vec3 ? GL_FLOAT
		   : type == GLAttributeType::Vec4 ? GL_FLOAT
										   : GL_NONE;
}

static GLenum ToGL(bool v) {
	return v ? GL_TRUE : GL_FALSE;
}

static GLenum ToGL(GLShaderType type) {
	return type == GLShaderType::Vertex		? GL_VERTEX_SHADER
		   : type == GLShaderType::Fragment ? GL_FRAGMENT_SHADER
											: GL_NONE;
}

static GLenum ToGL(GLDrawMode mode) {
	return mode == GLDrawMode::Triangles ? GL_TRIANGLES
										 : GL_NONE;
}

static GLenum ToGL(GLTextureType type) {
	return type == GLTextureType::Texture2D ? GL_TEXTURE_2D
											: GL_NONE;
}

static GLenum ToGL(GLTextureWrapParam param) {
	return param == GLTextureWrapParam::WrapS	? GL_TEXTURE_WRAP_S
		   : param == GLTextureWrapParam::WrapT ? GL_TEXTURE_WRAP_T
												: GL_NONE;
}

static GLenum ToGL(GLTextureWrap param) {
	return param == GLTextureWrap::Repeat ? GL_REPEAT
										  : GL_NONE;
}

static GLenum ToGL(GLTextureFilterParam filter) {
	return filter == GLTextureFilterParam::MinFilter   ? GL_TEXTURE_MIN_FILTER
		   : filter == GLTextureFilterParam::MagFilter ? GL_TEXTURE_MAG_FILTER
													   : GL_NONE;
}

static GLenum ToGL(GLTextureFilter filter) {
	return filter == GLTextureFilter::Linear			   ? GL_LINEAR
		   : filter == GLTextureFilter::LinearMipmapLinear ? GL_LINEAR_MIPMAP_LINEAR
														   : GL_NONE;
}

static GLenum ToGL(GLTextureFormat format) {
	return format == GLTextureFormat::Rgba		   ? GL_RGBA
		   : format == GLTextureFormat::Rgb		   ? GL_RGB
		   : format == GLTextureFormat::RedInteger ? GL_RED_INTEGER
												   : GL_NONE;
}

static GLenum ToGL(GLTextureInternalFormat format) {
	return format == GLTextureInternalFormat::Rgba		? GL_RGBA
		   : format == GLTextureInternalFormat::Rgba16F ? GL_RGBA16F
		   : format == GLTextureInternalFormat::R32I	? GL_R32I
														: GL_NONE;
}

static GLenum ToGL(GLDataType type) {
	return type == GLDataType::Float   ? GL_FLOAT
		   : type == GLDataType::Int   ? GL_INT
		   : type == GLDataType::UInt  ? GL_UNSIGNED_INT
		   : type == GLDataType::UByte ? GL_UNSIGNED_BYTE
									   : GL_NONE;
}

int GetChannelCount(GLTextureFormat format) {
	return format == GLTextureFormat::Rgb	 ? 3
		   : format == GLTextureFormat::Rgba ? 4
											 : 4;
}

static GLenum GetTextureFormat(int nrChannels) {
	return nrChannels == 1	 ? GL_RED
		   : nrChannels == 2 ? GL_RG
		   : nrChannels == 3 ? GL_RGB
		   : nrChannels == 4 ? GL_RGBA
							 : GL_NONE;
}

//

void GLManager::genBuffer(uint32_t &id) {
	glGenBuffers(1, &id);
}
void GLManager::deleteBuffer(uint32_t &id) {
	glDeleteBuffers(1, &id);
}
void GLManager::bindBuffer(BufferType type, uint32_t id) {
	if (m_boundBuffers[type] == id) {
		return;
	}
	glBindBuffer(ToGL(type), id);
}
void GLManager::bufferData(BufferType type, const void *data, uint32_t size, BufferUsage usage) {
	glBufferData(ToGL(type), size, data, ToGL(usage));
}
void GLManager::bufferSubdata(BufferType type, const void *data, uint32_t size, uint32_t offset) {
	glBufferSubData(ToGL(type), offset, size, data);
}

void GLManager::genVertexArray(uint32_t &id) {
	glGenVertexArrays(1, &id);
}
void GLManager::deleteVertexArray(uint32_t &id) {
	glDeleteVertexArrays(1, &id);
}
void GLManager::bindVertexArray(uint32_t id) {
	if (m_boundVertexArray == id) {
		return;
	}

	glBindVertexArray(id);
}

void GLManager::enableVertexAttribArray(uint32_t slot) {
	glEnableVertexAttribArray(slot);
}
void GLManager::disableVertexAttribArray(uint32_t slot) {
	glDisableVertexAttribArray(slot);
}
void GLManager::vertexAttribPointer(uint32_t slot, uint32_t elemCount, GLAttributeType type, bool normalized, uint32_t stride, uint64_t usedBytes) {
	glVertexAttribPointer(
		slot,
		elemCount,
		ToGL(type),
		ToGL(normalized),
		stride,
		(const void *)usedBytes);
}
uint32_t GLManager::createProgram() {
	return glCreateProgram();
}
void GLManager::attachShader(uint32_t id, uint32_t shader) {
	glAttachShader(id, shader);
}
void GLManager::linkProgram(uint32_t id) {
	glLinkProgram(id);

	{
		int success;
		char infoLog[512];
		glGetProgramiv(id, GL_LINK_STATUS, &success);

		if (!success) {
			glGetProgramInfoLog(id, 512, NULL, infoLog);
			Debug::Error("Shader Program compilation failed. \n {}\n", infoLog);
		}
	}
}
void GLManager::useProgram(uint32_t id) {
	glUseProgram(id);
}
void GLManager::deleteProgram(uint32_t id) {
	glDeleteProgram(id);
}
uint32_t GLManager::createShader(GLShaderType type) {
	return glCreateShader(ToGL(type));
}
void GLManager::shaderSource(uint32_t id, const std::string &src) {
	const char *source = src.c_str();
	glShaderSource(id, 1, &source, NULL);
}
void GLManager::compileShader(uint32_t id, GLShaderType shaderType) {
	glCompileShader(id);

	{
		int success;
		char infoLog[512];
		glGetShaderiv(id, GL_COMPILE_STATUS, &success);

		if (!success) {
			glGetShaderInfoLog(id, 512, NULL, infoLog);
			if (shaderType == GLShaderType::Vertex)
				Debug::Error("{} Shader compilation failed.\n {}\n", "Vertex", infoLog);
			else if (shaderType == GLShaderType::Fragment)
				Debug::Error("{} Shader compilation failed.\n {}\n", "Fragment", infoLog);
		}
	}
}
void GLManager::deleteShader(uint32_t id) {
	glDeleteShader(id);
}

void GLManager::drawElements(GLDrawMode mode, uint32_t indexCount) {
	glDrawElements(ToGL(mode), indexCount, GL_UNSIGNED_INT, nullptr);
}
void GLManager::drawArrays(GLDrawMode mode, uint32_t offset, uint32_t count) {
	glDrawArrays(ToGL(mode), offset, count);
}

void GLManager::genTextures(uint32_t &id) {
	glGenTextures(1, &id);
}
void GLManager::deleteTexture(uint32_t &id) {
	glDeleteTextures(1, &id);
}
void GLManager::activeTexture(uint32_t slot) {
	glActiveTexture(GL_TEXTURE0 + slot);
}
void GLManager::bindTexture(GLTextureType type, uint32_t id) {
	glBindTexture(ToGL(type), id);
}
void GLManager::texParameteri(GLTextureType type, GLTextureWrapParam param, GLTextureWrap wrap) {
	glTexParameteri(ToGL(type), ToGL(param), ToGL(wrap));
}
void GLManager::texParameteri(GLTextureType type, GLTextureFilterParam param, GLTextureFilter filter) {
	glTexParameteri(ToGL(type), ToGL(param), ToGL(filter));
}
void GLManager::generateMipmap(GLTextureType type) {
	glGenerateMipmap(ToGL(type));
}
void GLManager::texImage2D(GLTextureType type, int level, GLTextureInternalFormat internalFormat, int width, int height, int border, GLDataType t, const void *data, GLTextureFormat format) {
	glTexImage2D(ToGL(type), level, ToGL(internalFormat), width, height, border, ToGL(format), ToGL(t), data);
}
void GLManager::viewport(int x, int y, int w, int h) {
	glViewport(x, y, w, h);
}
void GLManager::genFramebuffer(uint32_t &id) {
	glGenFramebuffers(1, &id);
}
void GLManager::bindFramebuffer(uint32_t id) {
	glBindFramebuffer(GL_FRAMEBUFFER, id);
}
void GLManager::deleteFramebuffer(uint32_t id) {
	glDeleteFramebuffers(1, &id);
}
void GLManager::framebufferTexture2D(uint32_t attachmentSlot, uint32_t textureId) {
	glFramebufferTexture2D(GL_TEXTURE_2D, GL_COLOR_ATTACHMENT0 + attachmentSlot, GL_TEXTURE_2D, textureId, 0);
}
void GLManager::drawBuffers(const std::vector<int> &attachments) {
	std::vector<GLenum> buffers(attachments.size());
	for (const auto &attachment : attachments) {
		buffers.push_back(GL_COLOR_ATTACHMENT0 + attachment);
	}
	glDrawBuffers(buffers.size(), buffers.data());
}
bool GLManager::checkFramebufferStatus() {
	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	return status == GL_FRAMEBUFFER_COMPLETE;
}

void GLManager::setUnpackAlignment(int v) {
	glPixelStorei(GL_UNPACK_ALIGNMENT, v);
}
void GLManager::setDepthTest(bool v) {
	if(v) {
		glEnable(GL_BLEND);
	} else {
		glDisable(GL_BLEND);
	}
}

GLManager &GL() {
	static GLManager gl;
	return gl;
}
} // namespace gfx
} // namespace sowa
