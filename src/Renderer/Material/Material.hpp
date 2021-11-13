#pragma once
#include <glm/glm.hpp>
#include <memory>

#include "Renderer/Texture/Texture.hpp"

namespace Ease
{

struct Material
{
   std::shared_ptr<Texture> albedoTexture;
   glm::vec4 color;


   // metallic, normal map ...
};
   
} // namespace Ease
