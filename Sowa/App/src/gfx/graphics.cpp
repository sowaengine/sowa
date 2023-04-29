#include "graphics.hpp"

namespace sowa {
static gfx::IGraphics *s_graphics = nullptr;

namespace gfx {

IGraphics::IGraphics() {
}
IGraphics::~IGraphics() {
}

void IGraphics::SetInstance(IGraphics *graphics) {
	s_graphics = graphics;
}

} // namespace gfx

gfx::IGraphics &Graphics() {
	return *s_graphics;
}

} // namespace sowa
