#include "graphics_gl.hpp"

#include <vector>

#include "glad/glad.h"
#include "glm/gtc/matrix_transform.hpp"

#include "./font_gl.hpp"
#include "core/stats.hpp"
#include "debug.hpp"
#include "gfx/gl/glfuncs.hpp"

#define BATCH2D_MAX_VERTEX 1000 * 6 // 1000 rect
#define BATCH2D_MAX_TEXTURE 16

namespace sowa {
namespace gfx {

GraphicsGL::GraphicsGL() {
	int maxTextureSize = 0;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
	Debug::Log("Max texture size: {}", maxTextureSize);

	int maxTextureSlot = 0;
	glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &maxTextureSlot);
	Debug::Log("Max texture slot: {}", maxTextureSlot);

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

void GraphicsGL::DrawText(const std::string &text, IFont *font, float x, float y, mat4 transform, float scale) {
	DefaultUITextShader().Bind();
	m_UITextArray.Bind();

	std::string::const_iterator c;
	for (c = text.begin(); c != text.end(); c++) {
		GLFont::Character ch = reinterpret_cast<GLFont *>(font)->m_characters[*c];

		float xpos = x + ch.bearing.x * scale;
		float ypos = y - (ch.size.y - ch.bearing.y) * scale;

		float w = ch.size.x * scale;
		float h = ch.size.y * scale;

		glm::vec4 pos1 = transform * glm::vec4{xpos, ypos + h, 0.f, 1.f};
		glm::vec4 pos2 = transform * glm::vec4{xpos, ypos, 0.f, 1.f};
		glm::vec4 pos3 = transform * glm::vec4{xpos + w, ypos, 0.f, 1.f};
		glm::vec4 pos4 = transform * glm::vec4{xpos + w, ypos + h, 0.f, 1.f};

		float vertices[6][4] = {
			{pos1.x, pos1.y, 0.f, 0.f},
			{pos2.x, pos2.y, 0.f, 1.f},
			{pos3.x, pos3.y, 1.f, 1.f},

			{pos1.x, pos1.y, 0.f, 0.f},
			{pos3.x, pos3.y, 1.f, 1.f},
			{pos4.x, pos4.y, 1.f, 0.f}};

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

void GraphicsGL::DrawTextBlank(const std::string &text, IFont *font) {
	DrawText(text, font, 0.f, 0.f, mat4(1.f), 1.f);
}

void GraphicsGL::DrawTextWithTransform(const std::string &text, IFont *font, mat4 modelTransform) {
	DrawText(text, font, 0.f, 0.f, modelTransform, 1.f);
}

void GraphicsGL::DrawTextUI(const std::string &text, IFont *font, DrawTextUIArgs args) {

	std::string::const_iterator c;
	float x = 0.f;
	float y = 0.f;

	std::string currentText = "";
	if (args.drawMode == TextDrawMode::LetterWrap) {
		float width = 0.f;
		float tallestInLine = 0.f;

		for (c = text.begin(); c != text.end(); c++) {
			GLFont::Character ch = reinterpret_cast<GLFont *>(font)->m_characters[*c];
			if (ch.size.y > tallestInLine) {
				tallestInLine = ch.size.y;
			}

			currentText += *c;
			width += (ch.advance >> 6);

			if (args.align == TextAlign::Left) {
				x = 0.f;
			} else if (args.align == TextAlign::Center) {
				x = (args.targetWidth - width) * 0.5f;
			} else if (args.align == TextAlign::Right) {
				x = args.targetWidth - width;
			}

			if (args.targetWidth > 0.f && width >= args.targetWidth) {
				y -= tallestInLine;
				width = 0.f;
				tallestInLine = 0.f;

				DrawText(currentText, font, x, y, args.transform, 1.f);
				currentText = "";
			}

			if (c == text.end() - 1) {
				y -= tallestInLine;
				width = 0.f;
				tallestInLine = 0.f;

				if (currentText != "") {
					DrawText(currentText, font, x, y, args.transform, 1.f);
				}
			}
		}
	} else if (args.drawMode == TextDrawMode::WordWrap) {
		float width = 0.f;
		float tallestInLine = 0.f;

		for (c = text.begin(); c != text.end(); c++) {
			GLFont::Character ch = reinterpret_cast<GLFont *>(font)->m_characters[*c];
			if (ch.size.y > tallestInLine) {
				tallestInLine = ch.size.y;
			}

			currentText += *c;
			width += (ch.advance >> 6);

			if (args.align == TextAlign::Left) {
				x = 0.f;
			} else if (args.align == TextAlign::Center) {
				x = (args.targetWidth - width) * 0.5f;
			} else if (args.align == TextAlign::Right) {
				x = args.targetWidth - width;
			}

			if (args.targetWidth > 0.f && *c == ' ' && width >= args.targetWidth) {
				y -= tallestInLine;
				width = 0.f;
				tallestInLine = 0.f;

				DrawText(currentText, font, x, y, args.transform, 1.f);
				currentText = "";
			}

			if (c == text.end() - 1) {
				y -= tallestInLine;
				width = 0.f;
				tallestInLine = 0.f;
				x -= (ch.advance >> 6) * 0.5f;

				if (currentText != "") {
					DrawText(currentText, font, x, y, args.transform, 1.f);
				}
			}
		}
	} else if (args.drawMode == TextDrawMode::Stretch) {
		sowa::vec2f size = font->CalcTextSize(text);
		float scale = 1.f;
		if (args.targetWidth > 0.f) {
			scale = args.targetWidth / size.x;
		}

		DrawText(text, font, 0.f, 0.f, args.transform, scale);
	}
}

void GraphicsGL::Clear() {
	GL().clearColor(0.02f, 0.04f, 0.07f, 0.f);
	GL().clearColorBit();
	GL().clearDepthBit();
}

void GraphicsGL::SetViewportStyle(SetViewportStyleArgs args, rect *r) {
	if (args.mode == ViewportDrawMode_Stretch) {
		GL().viewport(0, 0, args.windowWidth, args.windowHeight);
		if (nullptr != r) {
			r->x = 0.f;
			r->y = 0.f;
			r->w = args.windowWidth;
			r->h = args.windowHeight;
		}
	} else if (args.mode == ViewportDrawMode_KeepRatio) {
		float windowRatio = (float)args.windowWidth / args.windowHeight;
		float videoRatio = (float)args.videoWidth / args.videoHeight;

		if (windowRatio > videoRatio) {
			float width = args.windowHeight * videoRatio;
			float height = args.windowHeight;
			float gap = args.windowWidth - width;

			float x = gap / 2.f;
			GL().viewport(x, 0.f, width, height);
			if (nullptr != r) {
				r->x = x;
				r->y = 0.f;
				r->w = width;
				r->h = height;
			}
		} else {
			float width = args.windowWidth;
			float height = args.windowWidth / videoRatio;
			float gap = args.windowHeight - height;

			float y = gap / 2.f;
			GL().viewport(0, y, width, height);
			if (nullptr != r) {
				r->x = 0.f;
				r->y = y;
				r->w = width;
				r->h = height;
			}
		}
	} else if (args.mode == ViewportDrawMode_KeepWidth) {
		float windowRatio = (float)args.windowWidth / args.windowHeight;
		float videoRatio = (float)args.videoWidth / args.videoHeight;

		float width = args.windowWidth;
		float height = args.windowWidth / videoRatio;
		float gap = args.windowHeight - height;

		float y = gap / 2.f;
		GL().viewport(0, y, width, height);
		if (nullptr != r) {
			r->x = 0.f;
			r->y = y;
			r->w = width;
			r->h = height;
		}
	} else if (args.mode == ViewportDrawMode_KeepHeight) {
		float windowRatio = (float)args.windowWidth / args.windowHeight;
		float videoRatio = (float)args.videoWidth / args.videoHeight;

		float height = args.windowHeight;
		float width = args.windowHeight * videoRatio;
		float gap = args.windowWidth - width;

		float x = gap / 2.f;
		GL().viewport(x, 0, width, height);
		if (nullptr != r) {
			r->x = x;
			r->y = 0.f;
			r->w = width;
			r->h = height;
		}
	} else if (args.mode == ViewportDrawMode_Contain) {
		float windowRatio = (float)args.windowWidth / args.windowHeight;
		float videoRatio = (float)args.videoWidth / args.videoHeight;

		if (windowRatio < videoRatio) {
			float width = args.windowHeight * videoRatio;
			float height = args.windowHeight;
			float gap = args.windowWidth - width;

			float x = gap / 2.f;
			GL().viewport(gap / 2, 0, width, height);
			if (nullptr != r) {
				r->x = x;
				r->y = 0.f;
				r->w = width;
				r->h = height;
			}
		} else {
			float width = args.windowWidth;
			float height = args.windowWidth / videoRatio;
			float gap = args.windowHeight - height;

			float y = gap / 2.f;
			GL().viewport(0, gap / 2, width, height);
			if (nullptr != r) {
				r->x = 0.f;
				r->y = y;
				r->w = width;
				r->h = height;
			}
		}
	}
}

} // namespace gfx
} // namespace sowa