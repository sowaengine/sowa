#include "resource/texture/image_texture.hpp"
#include "resource/resource_loader.hpp"

namespace sowa {
template <>
std::shared_ptr<sowa::ImageTexture> ResourceLoaderImpl<sowa::ImageTexture>::Load(const unsigned char *data, size_t size) {
	std::shared_ptr<sowa::ImageTexture> tex = std::make_shared<sowa::ImageTexture>();
	if (!tex->m_texture.Load2D(data, size))
		return nullptr;
	return tex;
}

FileBuffer ImageTexture::SaveImpl(object_type * o) {
	Debug::Error("ImageTexture::SaveImpl not implemented");

	/*
	 * tex.saveFlags.format = [png, jpg..., tex(yaml)] 
	*/
	return FileBuffer();
}
bool ImageTexture::LoadImpl(object_type* out, const FileBuffer& doc) {
	ImageTexture* obj = reinterpret_cast<ImageTexture*>(out);

	bool success = obj->m_texture.Load2D(doc.Data().data(), doc.Data().size());
	return success;
}

ImageTexture::ImageTexture() {
	m_type = Typename();
}

ImageTexture::~ImageTexture() {
}
} // namespace sowa