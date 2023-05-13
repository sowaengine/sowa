#ifndef SW_FONT_HPP_
#define SW_FONT_HPP_

#include <cstdint>

namespace sowa {
namespace gfx {
class IFont {
    public:
        virtual void LoadTTF(unsigned char* data, size_t size) = 0;
        virtual uint32_t GetGlyphTextureID(int glyph) = 0;
    private:

};
} // namespace gfx
} // namespace sowa

#endif // SW_FONT_HPP_