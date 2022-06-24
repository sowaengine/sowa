#ifndef _E_SPRITERENDERER_HPP__
#define _E_SPRITERENDERER_HPP__

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
        bool& Visible() { return m_Visible; }
    private:
        uint32_t m_TextureID;
        bool m_Visible = true;
    };
}

#endif