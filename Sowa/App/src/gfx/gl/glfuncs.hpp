#ifndef SW_GL_HPP_
#define SW_GL_HPP_

#include <map>
#include <vector>

#include "buffer_gl.hpp"
#include "shader_gl.hpp"
#include "texture_gl.hpp"
#include "vertex_array_gl.hpp"

namespace sowa {
namespace gfx {

enum class GLDrawMode {
	Triangles = 0,
};

enum class GLTextureWrapParam {
	WrapS = 0,
	WrapT,
};

enum class GLTextureFilterParam {
	MinFilter = 0,
	MagFilter,
};

enum class GLTextureWrap {
	Repeat = 0,
};

enum class GLTextureFilter {
	Linear = 0,
	Nearest,
	LinearMipmapLinear,
};

int GetChannelCount(GLTextureFormat format);

class GLManager {
  public:
	void genBuffer(uint32_t &id);
	void deleteBuffer(uint32_t &id);
	void bindBuffer(BufferType type, uint32_t id);
	void bufferData(BufferType type, const void *data, uint32_t size, BufferUsage usage);
	void bufferSubdata(BufferType type, const void *data, uint32_t size, uint32_t offset);

	void genVertexArray(uint32_t &id);
	void deleteVertexArray(uint32_t &id);
	void bindVertexArray(uint32_t id);

	void enableVertexAttribArray(uint32_t slot);
	void disableVertexAttribArray(uint32_t slot);
	void vertexAttribPointer(uint32_t slot, uint32_t elemCount, GLAttributeType type, bool normalized, uint32_t stride, uint64_t usedBytes);

	uint32_t createProgram();
	void attachShader(uint32_t id, uint32_t shader);
	void linkProgram(uint32_t id);
	void useProgram(uint32_t id);
	void deleteProgram(uint32_t id);
	uint32_t createShader(GLShaderType type);
	void shaderSource(uint32_t id, const std::string &src);
	void compileShader(uint32_t id, GLShaderType shaderType);
	void deleteShader(uint32_t id);

	void drawElements(GLDrawMode mode, uint32_t indexCount);
	void drawArrays(GLDrawMode mode, uint32_t offset, uint32_t count);

	void genTextures(uint32_t &id);
	void deleteTexture(uint32_t &id);
	void activeTexture(uint32_t slot);
	void bindTexture(GLTextureType type, uint32_t id);
	void texParameteri(GLTextureType type, GLTextureWrapParam param, GLTextureWrap wrap);
	void texParameteri(GLTextureType type, GLTextureFilterParam param, GLTextureFilter filter);
	void generateMipmap(GLTextureType type);
	void texImage2D(GLTextureType type, int level, GLTextureInternalFormat internalFormat, int width, int height, int border, GLDataType t, const void *data, GLTextureFormat format);

	void viewport(int x, int y, int w, int h);
	void genFramebuffer(uint32_t &id);
	void bindFramebuffer(uint32_t id);
	void deleteFramebuffer(uint32_t id);
	void framebufferTexture2D(uint32_t attachmentSlot, uint32_t textureId);
	void drawBuffers(const std::vector<int>& attachments);
	bool checkFramebufferStatus();

	void setUnpackAlignment(int);
	void setDepthTest(bool);

	void clearColor(float r, float g, float b, float a);
	void clearColorBit();
	void clearDepthBit();

	int getUniformLocation(uint32_t programID, const std::string& name);
	void uniform1i(int location, int i);

	GLManager();
  private:
	std::map<BufferType, uint32_t> m_boundBuffers{};
	uint32_t m_boundVertexArray = 0;
	uint32_t m_boundShader = 0;

};

GLManager &GL();

} // namespace gfx
} // namespace sowa

#endif // SW_GL_HPP_