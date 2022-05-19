#ifndef TRANSFORM2D_H
#define TRANSFORM2D_H

#pragma once

#include <glm/glm.hpp>

namespace Ease::Component
{
    class Transform2D
    {
    public:
        Transform2D();
        ~Transform2D();
        
        glm::vec2& Position() { return m_Position; }
        glm::vec2& Scale() { return m_Scale; }
        float& Rotation() { return m_Rotation; }
        int& ZIndex() { return m_ZIndex; }
    private:
        glm::vec2 m_Position;
        glm::vec2 m_Scale;
        float m_Rotation;
        int m_ZIndex;
    };
}

#endif