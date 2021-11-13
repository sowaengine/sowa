#include "Matrix.hpp"

namespace Ease::Matrix
{

bool decomposeMat4(const glm::mat4& transform, glm::vec3& translation, glm::vec3& rotation, glm::vec3& scale)
{
   glm::vec3 skew; glm::vec4 perspective;

   glm::mat4 rotmat = glm::mat4(1.f);
   rotmat = glm::rotate(rotmat, glm::radians(rotation.z), {0.f, 0.f, 1.f});
   glm::quat rot = glm::quat_cast(rotmat);

   glm::decompose(transform, scale, rot, translation, skew, perspective);
   rotation = glm::eulerAngles(rot);
   return true;

   translation = transform[3];
   for(int i = 0; i < 3; i++)
      scale[i] = glm::length(glm::vec3(transform[i]));
   
   const glm::mat3 rotMtx(
      glm::vec3(transform[0]) / scale[0],
      glm::vec3(transform[1]) / scale[1],
      glm::vec3(transform[2]) / scale[2]);

   rotation = glm::eulerAngles( glm::quat_cast(transform) );

   return true;
}
   
} // namespace Ease::Matrix
