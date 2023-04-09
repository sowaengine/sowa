#include "resource/texture/image_texture.hpp"
#include "resource/resource_loader.hpp"

namespace Sowa {
template <>
std::shared_ptr<Sowa::ImageTexture> ResourceLoaderImpl<Sowa::ImageTexture>::Load(const unsigned char *data, size_t size) {
	std::shared_ptr<Sowa::ImageTexture> tex = std::make_shared<Sowa::ImageTexture>();
	if (!tex->_texture.Load2DFromMemory(data, size))
		return nullptr;
	return tex;
}

FileBuffer ImageTexture::SaveImpl(ObjectType * o) {
	Debug::Error("ImageTexture::SaveImpl not implemented");

	/*
	 * tex.saveFlags.format = [png, jpg..., tex(yaml)] 
	*/
	return FileBuffer();
}
bool ImageTexture::LoadImpl(ObjectType* out, const FileBuffer& doc) {
	ImageTexture* obj = reinterpret_cast<ImageTexture*>(out);

	bool success = obj->_texture.Load2DFromMemory(doc.Data().data(), doc.Data().size());
	return success;
}

ImageTexture::ImageTexture() {
	m_Type = Typename();
}

ImageTexture::~ImageTexture() {
}
} // namespace Sowa