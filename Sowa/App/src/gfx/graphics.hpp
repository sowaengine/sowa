#ifndef SW_GRAPHICS_HPP_
#define SW_GRAPHICS_HPP_

#include "math/math.hpp"
#include "shader.hpp"

namespace sowa {
class Application;

namespace gfx {

class IFont;

enum ViewportDrawMode {
	ViewportDrawMode_None = 0,
	ViewportDrawMode_KeepRatio,
	ViewportDrawMode_KeepWidth,
	ViewportDrawMode_KeepHeight,
	ViewportDrawMode_Stretch,
	ViewportDrawMode_Contain,
};

struct SetViewportStyleArgs {
	// Any ViewportDrawMode other than none will call glviewport
	ViewportDrawMode mode;

	int windowWidth;
	int windowHeight;

	int videoWidth;
	int videoHeight;
};

enum class TextAlign {
	Center = 0,
	Left,
	Right
};

enum class TextDrawMode {
	WordWrap = 0,
	LetterWrap,
	Stretch
};

struct DrawTextUIArgs {
	float targetWidth;
	TextDrawMode drawMode;
	TextAlign align;
	mat4 transform = mat4(1.f);
};


struct BatchVertex {
	float x;
	float y;
	float z;
	float r;
	float g;
	float b;
	float a;
	float uvX;
	float uvY;
	float textureId;

	BatchVertex(float x, float y, float z, float r, float g, float b, float a, float uvX, float uvY, float textureId) {
		this->x = x;
		this->y = y;
		this->z = z;
		this->r = r;
		this->g = g;
		this->b = b;
		this->a = a;
		this->uvX = uvX;
		this->uvY = uvY;
		this->textureId = textureId;
	}
};

class IGraphics {
  public:
	virtual void SetDepthTest(bool) = 0;

	virtual IShader &Default2DShader() = 0;
	virtual IShader &DefaultSolidColorShader() = 0;
	virtual IShader &DefaultFullscreenShader() = 0;
	virtual IShader &DefaultUITextShader() = 0;
	virtual IShader &DefaultBatch2DShader() = 0;

	virtual void DrawQuad() = 0;
	virtual void DrawFullscreenQuad() = 0;
	virtual void DrawText(const std::string &text, IFont *font, float x, float y, mat4 transform, float scale) = 0;
	virtual void DrawTextBlank(const std::string &text, IFont *font) = 0;
	virtual void DrawTextWithTransform(const std::string &text, IFont *font, mat4 modelTransform) = 0;
	virtual void DrawTextUI(const std::string &text, IFont *font, DrawTextUIArgs args) = 0;

	virtual void SetViewportStyle(SetViewportStyleArgs args) = 0;
	virtual void Clear() = 0;

	virtual void Batch2DBegin() = 0;
	virtual void Batch2DPushQuad(BatchVertex vertices[4]) = 0;
	virtual void Batch2DEnd() = 0;

  protected:
	friend class sowa::Application;

	IGraphics();
	~IGraphics();

	static void SetInstance(IGraphics *);
};

} // namespace gfx

gfx::IGraphics &Graphics();
} // namespace sowa

#endif