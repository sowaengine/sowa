#ifndef SPRITERENDERER2D_H
#define SPRITERENDERER2D_H

#pragma once
#include <stdint.h>

namespace Ease::Component
{
    class SpriteRenderer2D
    {
    public:
        SpriteRenderer2D();
        ~SpriteRenderer2D();


        uint32_t& TextureID() { return m_TextureID; }
    private:
        uint32_t m_TextureID;
    };
}

#endif