#include "resource/font/font.hpp"
#include "resource/resource_loader.hpp"

#include "core/application.hpp"
#include "Core/nm_Renderer.hpp"

namespace sowa {
template <>
std::shared_ptr<sowa::Font> ResourceLoaderImpl<sowa::Font>::Load(const unsigned char *data, size_t size) {
	std::shared_ptr<sowa::Font> tex = std::make_shared<sowa::Font>();
	// if (!Application::get_singleton().RendererHandle()->LoadFont(&tex->_Font, data, size))
	// 	return nullptr;
	return tex;
}

Font::Font() {
}

Font::~Font() {
}
} // namespace sowa