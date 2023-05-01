#ifndef SW_GRAPHICS_HPP_
#define SW_GRAPHICS_HPP_

#include "shader.hpp"

namespace sowa {
class Application;

namespace gfx {

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
	
class IGraphics {
    public:

    virtual void SetDepthTest(bool) = 0;
	
	virtual IShader& Default2DShader() = 0;
	virtual IShader& DefaultFullscreenShader() = 0;

	virtual void DrawQuad() = 0;
	virtual void DrawFullscreenQuad() = 0;
	virtual void SetViewportStyle(SetViewportStyleArgs args) = 0;
	virtual void Clear() = 0;

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