#include "Resource/Texture/Texture.hpp"
#include "Resource/ResourceLoader.hpp"

namespace Ease {
template <>
std::shared_ptr<Ease::ImageTexture> ResourceLoaderImpl<Ease::ImageTexture>::Load(unsigned char *data, size_t size) {
	std::shared_ptr<Ease::ImageTexture> tex = std::make_shared<Ease::ImageTexture>();
	if (!tex->_texture.Load2DFromMemory(data, size))
		return nullptr;
	return tex;
}

ImageTexture::ImageTexture() {
}

ImageTexture::~ImageTexture() {
}
} // namespace Ease