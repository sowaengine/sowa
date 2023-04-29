#ifndef SW_GRAPHICS_HPP_
#define SW_GRAPHICS_HPP_

namespace sowa {
class Application;

namespace gfx {
class IGraphics {
    public:

    

  private:
	friend class sowa::Application;

	IGraphics();
	~IGraphics();

	static void SetInstance(IGraphics *);
};

} // namespace gfx

gfx::IGraphics &Graphics();
} // namespace sowa

#endif