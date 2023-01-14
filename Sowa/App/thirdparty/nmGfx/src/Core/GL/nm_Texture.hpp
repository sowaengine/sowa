#ifndef __NM_GFX_TEXTURE_HPP__
#define __NM_GFX_TEXTURE_HPP__

#include <stdint.h>
#include <string>

namespace nmGfx
{
    enum class TextureType
    {
        TEXTURE2D = 0,
        CUBEMAP = 1,
    };

    class Texture
    {
        public:
            struct CubemapImagePaths
            {
                std::string right;  // positive-x
                std::string left;   // negative-x
                std::string top;    // positive-y
                std::string bottom; // negavive-y
                std::string front;  // positive-z
                std::string back;   // negative-z

                std::string operator[](int index)
                {
                    std::string* paths[6] = { &right, &left, &top, &bottom, &front, &back };
                    if(index >= 0 && index < 6)
                        return *paths[index];
                    return "";
                }
            };
            
        public:
            Texture() = default;
            ~Texture();

            void Load2DFromFile(const char* path);
            bool Load2DFromMemory(unsigned char* data, size_t size);
            void LoadCubemapFromFiles(CubemapImagePaths paths);
            void Use(int slot = 0);

            
            // Setting values directly with pixel values
            inline int GetWidth()  { return _width; }
            inline int GetHeight()  { return _height; }
            inline int GetChannels()  { return _channels; }
			inline unsigned char *GetPixels() { return _pixels; }
			// Loads from raw pixel data
			void LoadFromData(unsigned char* data, int width, int height, int channels, TextureType type = TextureType::TEXTURE2D);

            unsigned int ID() { return _id; }
        private:
            int _width = 0;
            int _height = 0;
            int _channels = 0;
			unsigned char *_pixels{nullptr};

			unsigned int _id = 0;

			TextureType _type;

			friend class Renderer;
    };
} // namespace nmGfx


#endif // __NM_GFX_TEXTURE_HPP__