#ifndef __NM_GFX_MATERIAL_HPP__
#define __NM_GFX_MATERIAL_HPP__
#pragma once

#include <memory.h>
#include "glm/glm.hpp"

namespace nmGfx
{
    class Texture;

    struct Material
    {
        glm::vec4 albedo{1.0f, 1.0f, 1.0f, 1.0f};
        std::shared_ptr<Texture> albedo_tex{nullptr};
        float specular{0.f};
        std::shared_ptr<Texture> specular_tex{nullptr};
    };
    
} // namespace nmGfx


#endif