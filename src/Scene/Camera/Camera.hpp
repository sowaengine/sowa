#pragma once

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

namespace Ease::Comp
{
   struct __Cameraa
   {
      glm::vec2 size;

      glm::mat4 getProjection()
      {
         return glm::ortho(0.f, (float)size.x, 0.f, (float)size.y, -1.f, 1.f);
      }
   };
   
} // namespace Ease::Comp
