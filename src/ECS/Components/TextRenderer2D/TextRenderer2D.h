#ifndef TEXTRENDERER2D_H
#define TEXTRENDERER2D_H

#pragma once
#include <string>
#include <glm/glm.hpp>

namespace Ease::Component
{
    class TextRenderer2D
    {
    public:
        TextRenderer2D();
        TextRenderer2D(const std::string& text) : m_Text(text) {}
        ~TextRenderer2D();
        
        std::string& Text() { return m_Text; }
        glm::vec4& Color() { return m_Color; }
        float& FontSize() { return m_FontSize; }
    private:
        std::string m_Text;
        glm::vec4 m_Color{255.f, 255.f, 255.f, 255.f};
        float m_FontSize = 64.f;
    };
}

#endif