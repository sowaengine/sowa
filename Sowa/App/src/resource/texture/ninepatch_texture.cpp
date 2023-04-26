#include "resource/texture/ninepatch_texture.hpp"
#include "resource/resource_loader.hpp"

namespace sowa {
template <>
std::shared_ptr<sowa::NinePatchTexture> ResourceLoaderImpl<sowa::NinePatchTexture>::Load(const unsigned char *data, size_t size) {
	std::shared_ptr<sowa::NinePatchTexture> tex = std::make_shared<sowa::NinePatchTexture>();
	if (!tex->_texture.Load2DFromMemory(data, size))
		return nullptr;
	return tex;
}

NinePatchTexture::NinePatchTexture() {
}

NinePatchTexture::~NinePatchTexture() {
}
} // namespace sowa