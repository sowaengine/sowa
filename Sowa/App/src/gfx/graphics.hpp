#ifndef SW_GRAPHICS_HPP_
#define SW_GRAPHICS_HPP_

#include "shader.hpp"

namespace sowa {
class Application;

namespace gfx {
class IGraphics {
    public:

    virtual void SetDepthTest(bool) = 0;
	
	virtual IShader& Default2DShader() = 0;

	virtual void DrawQuad() = 0;
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