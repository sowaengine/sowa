#include "nm_Texture.hpp"

#include "glad/glad.h"
#include "stb_image.h"

namespace nmGfx
{
    static GLenum GetTextureType(TextureType type)
    {
        return type == TextureType::TEXTURE2D ? GL_TEXTURE_2D
               : type == TextureType::CUBEMAP ? GL_TEXTURE_CUBE_MAP
               : GL_NONE;
    }
    static GLenum GetTextureFormat(int nrChannels)
    {
        return nrChannels == 1 ? GL_RED
            : nrChannels == 2 ? GL_RG
            : nrChannels == 3 ? GL_RGB
            : nrChannels == 4 ? GL_RGBA
            : GL_NONE;
    }

    Texture::~Texture()
    {
        
    }

    void Texture::LoadFromData(unsigned char* data, int width, int height, int channels, TextureType type/* = TextureType::TEXTURE2D*/)
    {
		if (_pixels != nullptr) {
			stbi_image_free(_pixels);
			_pixels = nullptr;
		}

		_width = width;
		_height = height;
		_channels = channels;
		_pixels = data;
		_type = type;

		if (data) {
			// glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

			glGenTextures(1, &_id);
			glBindTexture(GetTextureType(_type), _id);

			glTexParameteri(GetTextureType(_type), GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GetTextureType(_type), GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexParameteri(GetTextureType(_type), GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTexParameteri(GetTextureType(_type), GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			glTexImage2D(GetTextureType(_type), 0, GL_RGB, _width, _height, 0, GetTextureFormat(_channels), GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GetTextureType(_type));
		} else {
#ifdef NMGFX_PRINT_MESSAGES
            printf("Error: Failed to load texture from pixel data.\n");
#endif
		}
	}

	void Texture::Load2DFromFile(const char *path) {
		_type = TextureType::TEXTURE2D;

		if (_pixels != nullptr) {
			stbi_image_free(_pixels);
			_pixels = nullptr;
		}

		_pixels = stbi_load(path, &_width, &_height, &_channels, 0);
		if (_pixels) {
			// glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

			glGenTextures(1, &_id);
			glBindTexture(GetTextureType(_type), _id);

			glTexParameteri(GetTextureType(_type), GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GetTextureType(_type), GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexParameteri(GetTextureType(_type), GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GetTextureType(_type), GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			glTexImage2D(GetTextureType(_type), 0, GL_RGBA, _width, _height, 0, GetTextureFormat(_channels), GL_UNSIGNED_BYTE, _pixels);
			glGenerateMipmap(GetTextureType(_type));
		} else {
#ifdef NMGFX_PRINT_MESSAGES
			printf("Error: Failed to load texture: %s\n", path);
#endif
		}
	}

	bool Texture::Load2DFromMemory(unsigned char *filedata, size_t size) {
		_type = TextureType::TEXTURE2D;

		if (_pixels != nullptr) {
			stbi_image_free(_pixels);
			_pixels = nullptr;
		}

		_pixels = stbi_load_from_memory(filedata, size, &_width, &_height, &_channels, 0);
		if (_pixels) {
			// glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

			glGenTextures(1, &_id);
			glBindTexture(GetTextureType(_type), _id);

			glTexParameteri(GetTextureType(_type), GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GetTextureType(_type), GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexParameteri(GetTextureType(_type), GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GetTextureType(_type), GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			glTexImage2D(GetTextureType(_type), 0, GL_RGBA, _width, _height, 0, GetTextureFormat(_channels), GL_UNSIGNED_BYTE, _pixels);
			glGenerateMipmap(GetTextureType(_type));
		} else {
#ifdef NMGFX_PRINT_MESSAGES
			printf("Error: Failed to load texture\n");
#endif
			return false;
		}
		return true;
	}

	void Texture::LoadCubemapFromFiles(CubemapImagePaths paths) {
		_type = TextureType::CUBEMAP;

		if (_pixels != nullptr) {
			stbi_image_free(_pixels);
			_pixels = nullptr;
		}

		glGenTextures(1, &_id);
		glBindTexture(GetTextureType(_type), _id);

		glTexParameteri(GetTextureType(_type), GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GetTextureType(_type), GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GetTextureType(_type), GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		glTexParameteri(GetTextureType(_type), GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GetTextureType(_type), GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		for (int i = 0; i < 6; i++) {
			unsigned char *data = stbi_load(paths[i].c_str(), &_width, &_height, &_channels, 0);
			if (data) {
				glTexImage2D(
					GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
					0, GL_RGB,
					_width, _height,
					0,
					GetTextureFormat(_channels),
					GL_UNSIGNED_BYTE,
					data);
				stbi_image_free(data);
			} else {
#ifdef NMGFX_PRINT_MESSAGES
                printf("Error: Failed to load texture: %s\n", paths[i].c_str());
#endif
			}
		}
	}

	void Texture::Use(int slot /*= 0*/) {
		glActiveTexture(GL_TEXTURE0 + slot);
		glBindTexture(GetTextureType(_type), _id);
	}
} // namespace nmGfx
