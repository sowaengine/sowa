#include "resource/texture/ninepatch_texture.hpp"
#include "resource/resource_loader.hpp"

namespace Sowa {
template <>
std::shared_ptr<Sowa::NinePatchTexture> ResourceLoaderImpl<Sowa::NinePatchTexture>::Load(unsigned char *data, size_t size) {
	std::shared_ptr<Sowa::NinePatchTexture> tex = std::make_shared<Sowa::NinePatchTexture>();
	if (!tex->_texture.Load2DFromMemory(data, size))
		return nullptr;
	return tex;
}

NinePatchTexture::NinePatchTexture() {
}

NinePatchTexture::~NinePatchTexture() {
}
} // namespace Sowa