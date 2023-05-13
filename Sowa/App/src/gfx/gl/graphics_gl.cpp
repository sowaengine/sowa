#include "graphics_gl.hpp"

#include <vector>

#include "debug.hpp"
#include "gfx/gl/glfuncs.hpp"
#include "glad/glad.h"

#include "./font_gl.hpp"

namespace sowa {
namespace gfx {

GraphicsGL::GraphicsGL() {
	int maxTextureSize = 0;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
	Debug::Log("Max texture size: {}", maxTextureSize);

	// 2d quad
	const std::vector<float> quad_data = {
		// vec2 position, vec2 uv
		-0.5f,
		0.5f,
		0.0f,
		1.0f,

		-0.5f,
		-0.5f,
		0.0f,
		0.0f,

		0.5f,
		-0.5f,
		1.0f,
		0.0f,

		0.5f,
		0.5f,
		1.0f,
		1.0f,
	};

	const std::vector<uint32_t> quad_indices = {
		0, 1, 2,
		0, 2, 3};

	m_default2dmesh.New();
	m_default2dmesh.ResetAttributes();
	m_default2dmesh.SetModelData(quad_data);
	m_default2dmesh.SetIndexData(quad_indices);
	m_default2dmesh.SetAttribute(0, GLAttributeType::Vec2);
	m_default2dmesh.SetAttribute(1, GLAttributeType::Vec2);
	m_default2dmesh.UploadAttributes();

	// Fullscreen quad
	const std::vector<float> fullscreen_quad_data = {
		// vec2 position, vec2 uv
		-1.0f,
		1.0f,
		0.0f,
		1.0f,

		-1.0f,
		-1.0f,
		0.0f,
		0.0f,

		1.0f,
		-1.0f,
		1.0f,
		0.0f,

		1.0f,
		1.0f,
		1.0f,
		1.0f,
	};

	const std::vector<uint32_t> fullscreen_quad_indices = {
		0, 1, 2,
		0, 2, 3};

	m_defaultFullscreenMesh.New();
	m_defaultFullscreenMesh.ResetAttributes();
	m_defaultFullscreenMesh.SetModelData(fullscreen_quad_data);
	m_defaultFullscreenMesh.SetIndexData(fullscreen_quad_indices);
	m_defaultFullscreenMesh.SetAttribute(0, GLAttributeType::Vec2);
	m_defaultFullscreenMesh.SetAttribute(1, GLAttributeType::Vec2);
	m_defaultFullscreenMesh.UploadAttributes();


	m_UITextArray.New();
	m_UITextBuffer.New(BufferType::VertexBuffer);

	m_UITextArray.Bind();
	m_UITextBuffer.Bind();

	m_UITextBuffer.BufferData(nullptr, sizeof(float) * 6 * 4, BufferUsage::DynamicDraw);

	m_UITextArray.ResetAttributes();
	m_UITextArray.SetAttribute(0, GLAttributeType::Vec2);
	m_UITextArray.SetAttribute(1, GLAttributeType::Vec2);
	m_UITextArray.UploadAttributes();

	m_UITextArray.Unbind();
}
GraphicsGL::~GraphicsGL() {
}

void GraphicsGL::SetDepthTest(bool v) {
	GL().setDepthTest(v);
}

IShader &GraphicsGL::Default2DShader() {
	return m_default2dshader;
}

IShader &GraphicsGL::DefaultSolidColorShader() {
	return m_defaultSolidColorShader;
}

IShader &GraphicsGL::DefaultFullscreenShader() {
	return m_defaultFullscreenShader;
}

IShader &GraphicsGL::DefaultUITextShader() {
	return m_defaultUITextShader;
}

void GraphicsGL::DrawQuad() {
	m_default2dmesh.Draw();
}
void GraphicsGL::DrawFullscreenQuad() {
	m_defaultFullscreenMesh.Draw();
}

void GraphicsGL::DrawText(const std::string& text, IFont* font) {
	DefaultUITextShader().Bind();
	m_UITextArray.Bind();

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);

	std::string::const_iterator c;
	float x = 0.f;
	float y = 0.f;
	float scale = 1.f;
	for(c = text.begin(); c != text.end(); c++) {
		GLFont::Character ch = reinterpret_cast<GLFont*>(font)->m_characters[*c];
	
		float xpos = x + ch.bearing.x * scale;
		float ypos = y - (ch.size.y - ch.bearing.y) * scale;

		float w = ch.size.x * scale;
		float h = ch.size.y * scale;
		float vertices[6][4] = {
			{ xpos,     ypos + h,     0.f, 0.f },
			{ xpos,     ypos,         0.f, 1.f },
			{ xpos + w, ypos,         1.f, 1.f },

			{ xpos,      ypos + h,    0.f, 0.f },
			{ xpos + w,  ypos,        1.f, 1.f },
			{ xpos + w,  ypos + h,    1.f, 0.f }
		};

		DefaultUITextShader().UniformTexture("uTexture", ch.textureId, 0);

		m_UITextBuffer.Bind();
		m_UITextBuffer.BufferSubdata(vertices, sizeof(vertices), 0);
		m_UITextBuffer.Unbind();

		if (ch.textureId != 0) {
			GL().drawArrays(GLDrawMode::Triangles, 0, 6);
		}

		x += (ch.advance >> 6) * scale;
	}
	m_UITextArray.Unbind();
}

void GraphicsGL::Clear() {
	GL().clearColor(0.02f, 0.04f, 0.07f, 0.f);
	GL().clearColorBit();
	GL().clearDepthBit();
}

void GraphicsGL::SetViewportStyle(SetViewportStyleArgs args) {
	if (args.mode == ViewportDrawMode_Stretch) {
		GL().viewport(0, 0, args.windowWidth, args.windowHeight);
	} else if (args.mode == ViewportDrawMode_KeepRatio) {
		float windowRatio = (float)args.windowWidth / args.windowHeight;
		float videoRatio = (float)args.videoWidth / args.videoHeight;

		if (windowRatio > videoRatio) {
			float width = args.windowHeight * videoRatio;
			float height = args.windowHeight;
			float gap = args.windowWidth - width;

			GL().viewport(gap / 2, 0, width, height);
		} else {
			float width = args.windowWidth;
			float height = args.windowWidth / videoRatio;
			float gap = args.windowHeight - height;

			GL().viewport(0, gap / 2, width, height);
		}
	} else if (args.mode == ViewportDrawMode_KeepWidth) {
		float windowRatio = (float)args.windowWidth / args.windowHeight;
		float videoRatio = (float)args.videoWidth / args.videoHeight;

		float width = args.windowWidth;
		float height = args.windowWidth / videoRatio;
		float gap = args.windowHeight - height;

		GL().viewport(0, gap / 2, width, height);
	} else if (args.mode == ViewportDrawMode_KeepHeight) {
		float windowRatio = (float)args.windowWidth / args.windowHeight;
		float videoRatio = (float)args.videoWidth / args.videoHeight;

		float height = args.windowHeight;
		float width = args.windowHeight * videoRatio;
		float gap = args.windowWidth - width;

		GL().viewport(gap / 2, 0, width, height);
	} else if (args.mode == ViewportDrawMode_Contain) {
		float windowRatio = (float)args.windowWidth / args.windowHeight;
		float videoRatio = (float)args.videoWidth / args.videoHeight;

		if (windowRatio < videoRatio) {
			float width = args.windowHeight * videoRatio;
			float height = args.windowHeight;
			float gap = args.windowWidth - width;

			GL().viewport(gap / 2, 0, width, height);
		} else {
			float width = args.windowWidth;
			float height = args.windowWidth / videoRatio;
			float gap = args.windowHeight - height;

			GL().viewport(0, gap / 2, width, height);
		}
	}
}

} // namespace gfx
} // namespace sowa