#pragma once

#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/matrix_decompose.hpp"

namespace Ease::Matrix
{
   
   bool decomposeMat4(const glm::mat4& transform, glm::vec3& translation, glm::vec3& rotation, glm::vec3& scale);
   

} // namespace Ease::Matrix
