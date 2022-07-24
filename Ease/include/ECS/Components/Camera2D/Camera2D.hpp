#ifndef _E_CAMERA2D_HPP__
#define _E_CAMERA2D_HPP__

#pragma once

#include <glm/glm.hpp>
#include "raylib.h"

namespace Ease::Component
{
    class Camera2D
    {
    public:
        Camera2D();
        ~Camera2D();
        
        float& Zoom() { return m_Camera.zoom; }

        bool& Current() { return m_Current; }
    private:

        ::Camera2D m_Camera;
        bool m_Current = false;
    };
}

#endif