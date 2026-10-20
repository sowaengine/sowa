// Explicit desktop integration target: requires an OpenGL-capable login session.
#include <doctest/doctest.h>
#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#include "render/gl/gl_render_target.hpp"
#include "render/gl/gl_texture.hpp"
#include <GLFW/glfw3.h>

using namespace sowa;

TEST_CASE("rectangle ID readback returns red-channel tokens and restores GL pack state") {
	REQUIRE(glfwInit());
	glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
	GLFWwindow *window = glfwCreateWindow(32, 32, "Sowa picking integration", nullptr, nullptr);
	REQUIRE(window);
	glfwMakeContextCurrent(window);
	REQUIRE(gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)));
	{
		GLTextureStore textures;
		GLRenderTargetStore targets;
		RenderTargetDesc desc;
		desc.width = 8;
		desc.height = 6;
		desc.unsignedIntegerAttachment = true;
		auto target = targets.Create(desc, textures);
		REQUIRE(target.IsValid());
		targets.Bind(target);
		targets.ClearUInt(target, 1, 7);
		glEnable(GL_SCISSOR_TEST);
		glScissor(2, 1, 3, 2);
		targets.ClearUInt(target, 1, 42);
		glDisable(GL_SCISSOR_TEST);
		glReadBuffer(GL_COLOR_ATTACHMENT0);
		GLuint packBuffer = 0;
		glGenBuffers(1, &packBuffer);
		glBindBuffer(GL_PIXEL_PACK_BUFFER, packBuffer);
		glBufferData(GL_PIXEL_PACK_BUFFER, 1024, nullptr, GL_STREAM_READ);
		glPixelStorei(GL_PACK_ALIGNMENT, 8);
		glPixelStorei(GL_PACK_ROW_LENGTH, 11);
		glPixelStorei(GL_PACK_SKIP_ROWS, 1);
		glPixelStorei(GL_PACK_SKIP_PIXELS, 1);
		const auto pixels = targets.ReadUIntRect(target, 1, 1, 0, 5, 4);
		REQUIRE(pixels.size() == 20);
		for (int y = 0; y < 4; ++y) {
			for (int x = 0; x < 5; ++x) {
				CHECK(pixels[y * 5 + x] ==
					static_cast<u32>(x >= 1 && x <= 3 && y >= 1 && y <= 2 ? 42 : 7));
			}
		}
		GLint value = 0;
		glGetIntegerv(GL_PACK_ALIGNMENT, &value);
		CHECK(value == 8);
		glGetIntegerv(GL_PACK_ROW_LENGTH, &value);
		CHECK(value == 11);
		glGetIntegerv(GL_PACK_SKIP_ROWS, &value);
		CHECK(value == 1);
		glGetIntegerv(GL_PACK_SKIP_PIXELS, &value);
		CHECK(value == 1);
		glGetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING, &value);
		CHECK(static_cast<GLuint>(value) == packBuffer);
		glGetIntegerv(GL_READ_BUFFER, &value);
		CHECK(value == GL_COLOR_ATTACHMENT0);
		CHECK(targets.ReadUIntRect(target, 1, -1, 0, 5, 4).empty());
		CHECK(targets.ReadUIntRect(target, 1, 0, 0, 9, 6).empty());
		CHECK(targets.ReadUIntRect(target, 0, 0, 0, 1, 1).empty());
		CHECK(targets.ReadUIntRect({}, 1, 0, 0, 1, 1).empty());
		CHECK(glGetError() == GL_NO_ERROR);
		glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
		glDeleteBuffers(1, &packBuffer);
		targets.DestroyAll(textures);
		textures.DestroyAll();
	}
	glfwDestroyWindow(window);
	glfwTerminate();
}
