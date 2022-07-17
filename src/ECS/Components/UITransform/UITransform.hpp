#ifndef _E_UITRANSFORM_HPP__
#define _E_UITRANSFORM_HPP__

#pragma once

#include <glm/glm.hpp>

namespace Ease::Component
{
    class UITransform
    {
    public:
        UITransform();
        ~UITransform();
        
        glm::vec2& Position() { return m_Position; }
        glm::vec2& Scale() { return m_Scale; }
        glm::vec2& Size() { return m_Size; }
        float& Rotation() { return m_Rotation; }
        int& ZIndex() { return m_ZIndex; }
    private:
        glm::vec2 m_Position;
        glm::vec2 m_Scale;
        glm::vec2 m_Size;
        float m_Rotation;
        int m_ZIndex;
    };
}

#endif