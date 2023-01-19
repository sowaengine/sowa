#include "resource/texture/texture.hpp"
#include "resource/resource_loader.hpp"

namespace Sowa {
template <>
std::shared_ptr<Sowa::ImageTexture> ResourceLoaderImpl<Sowa::ImageTexture>::Load(unsigned char *data, size_t size) {
	std::shared_ptr<Sowa::ImageTexture> tex = std::make_shared<Sowa::ImageTexture>();
	if (!tex->_texture.Load2DFromMemory(data, size))
		return nullptr;
	return tex;
}

ImageTexture::ImageTexture() {
}

ImageTexture::~ImageTexture() {
}
} // namespace Sowa