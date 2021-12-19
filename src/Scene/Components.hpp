#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

#include <memory>
#include "Resource/Texture/Texture.hpp"


namespace Ease::Component
{
   
struct Transform2D
{
   glm::vec2 position = {0.f, 0.f};
   glm::vec2 scale = {1.f, 1.f};
   float rotation = 0.f;
   float zIndex = 0.f;

   
   glm::mat4 Transform(float scaleMultX = 1.0f, float scaleMultY = 1.0f, float zPosition = 0.f) const
   {
      glm::mat4 rotation = glm::toMat4(glm::quat({0.f, 0.f, this->rotation}));

      return glm::translate(glm::mat4(1.0f), {position.x, position.y, zPosition})
         * rotation
         * glm::scale(glm::mat4(1.0f), {scale.x * scaleMultX, scale.y * scaleMultY, 1.0f});
   }
};

struct Sprite2D
{
   glm::vec3 color = {0.2f, 0.5f, 1.f};
   std::shared_ptr<Texture> texture;
};

struct Camera2D
{
   bool current = false;
};


} // namespace Ease::Component
