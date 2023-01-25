#include "resource/font/font.hpp"
#include "resource/resource_loader.hpp"

#include "core/application.hpp"
#include "Core/nm_Renderer.hpp"

namespace Sowa {
template <>
std::shared_ptr<Sowa::Font> ResourceLoaderImpl<Sowa::Font>::Load(unsigned char *data, size_t size) {
	std::shared_ptr<Sowa::Font> tex = std::make_shared<Sowa::Font>();
	if (!Application::get_singleton().RendererHandle()->LoadFont(&tex->_Font, data, size))
		return nullptr;
	return tex;
}

Font::Font() {
}

Font::~Font() {
}
} // namespace Sowa