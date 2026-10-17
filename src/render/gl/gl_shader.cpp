#include "render/gl/gl_shader.hpp"

#include "render/gl/gl.hpp"

#include "core/log.hpp"

#include <string>

namespace sowa {
namespace {

#if defined(__EMSCRIPTEN__)
// Shaders are authored once as desktop GLSL (#version 330 core). WebGL2
// needs GLSL ES 3.00, which is the same language for everything the engine
// uses apart from the version line and default precision.
std::string toGlslEs(std::string_view source) {
	constexpr std::string_view kHeader = "#version 300 es\n"
										 "precision highp float;\n"
										 "precision highp int;\n"
										 "precision highp usampler2D;\n";
	const size_t version = source.find("#version");
	if (version == std::string_view::npos) {
		return std::string(kHeader) + std::string(source);
	}
	const size_t lineEnd = source.find('\n', version);
	std::string result(source.substr(0, version));
	result += kHeader;
	if (lineEnd != std::string_view::npos) {
		result += source.substr(lineEnd + 1);
	}
	return result;
}
#endif

bool compileStage(GLenum type, std::string_view source, u32 &outId) {
#if defined(__EMSCRIPTEN__)
	const std::string converted = toGlslEs(source);
	source = converted;
#endif
	outId = glCreateShader(type);
	const char *src = source.data();
	const GLint length = static_cast<GLint>(source.size());
	glShaderSource(outId, 1, &src, &length);
	glCompileShader(outId);

	GLint success = 0;
	glGetShaderiv(outId, GL_COMPILE_STATUS, &success);
	if (!success) {
		char infoLog[1024];
		glGetShaderInfoLog(outId, sizeof(infoLog), nullptr, infoLog);
		Log::Error("{} shader compile error: {}", type == GL_VERTEX_SHADER ? "vertex" : "fragment",
			infoLog);
		glDeleteShader(outId);
		outId = 0;
		return false;
	}
	return true;
}

} // namespace

ShaderHandle GLShaderStore::Create(std::string_view vertexSource, std::string_view fragmentSource) {
	u32 vertexShader = 0;
	u32 fragmentShader = 0;
	if (!compileStage(GL_VERTEX_SHADER, vertexSource, vertexShader)) {
		return {};
	}
	if (!compileStage(GL_FRAGMENT_SHADER, fragmentSource, fragmentShader)) {
		glDeleteShader(vertexShader);
		return {};
	}

	const u32 program = glCreateProgram();
	glAttachShader(program, vertexShader);
	glAttachShader(program, fragmentShader);
	glLinkProgram(program);

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	GLint success = 0;
	glGetProgramiv(program, GL_LINK_STATUS, &success);
	if (!success) {
		char infoLog[1024];
		glGetProgramInfoLog(program, sizeof(infoLog), nullptr, infoLog);
		Log::Error("Shader program link error: {}", infoLog);
		glDeleteProgram(program);
		return {};
	}

	u32 slotIndex = 0;
	bool found = false;
	for (; slotIndex < _slots.size(); ++slotIndex) {
		if (!_slots[slotIndex].occupied) {
			found = true;
			break;
		}
	}
	if (!found) {
		slotIndex = static_cast<u32>(_slots.size());
		_slots.push_back({});
	}

	Slot &slot = _slots[slotIndex];
	slot.glProgram = program;
	slot.occupied = true;

	return ShaderHandle{slotIndex + 1, slot.version};
}

void GLShaderStore::Destroy(ShaderHandle handle) {
	Slot *slot = resolve(handle);
	if (!slot) {
		return;
	}

	if (slot->glProgram != 0) {
		glDeleteProgram(slot->glProgram);
		slot->glProgram = 0;
	}
	slot->occupied = false;
	++slot->version;
}

void GLShaderStore::Bind(ShaderHandle handle) const {
	if (!handle.IsValid()) {
		glUseProgram(0);
		return;
	}

	const Slot *slot = resolve(handle);
	if (!slot) {
		Log::Warn("GLShaderStore::Bind: invalid or stale handle");
		glUseProgram(0);
		return;
	}

	glUseProgram(slot->glProgram);
}

void GLShaderStore::DestroyAll() {
	for (Slot &slot : _slots) {
		if (slot.occupied && slot.glProgram != 0) {
			glDeleteProgram(slot.glProgram);
		}
		slot.glProgram = 0;
		slot.occupied = false;
		++slot.version;
	}
	glUseProgram(0);
}

u32 GLShaderStore::ProgramId(ShaderHandle handle) const {
	const Slot *slot = resolve(handle);
	return slot ? slot->glProgram : 0;
}

GLShaderStore::Slot *GLShaderStore::resolve(ShaderHandle handle) {
	return const_cast<Slot *>(static_cast<const GLShaderStore *>(this)->resolve(handle));
}

const GLShaderStore::Slot *GLShaderStore::resolve(ShaderHandle handle) const {
	if (!handle.IsValid()) {
		return nullptr;
	}
	const u32 index = handle.id - 1;
	if (index >= _slots.size()) {
		return nullptr;
	}
	const Slot &slot = _slots[index];
	if (!slot.occupied || slot.version != handle.version) {
		return nullptr;
	}
	return &slot;
}

} // namespace sowa
