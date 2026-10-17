#include "render/graphics_device.hpp"

#include "render/gl/gl_graphics_device.hpp"

namespace sowa {

Scope<GraphicsDevice> CreateGraphicsDevice(GraphicsAPI api) {
	switch (api) {
	case GraphicsAPI::OpenGL:
		return MakeScope<GLGraphicsDevice>();
	}
	return nullptr;
}

} // namespace sowa
