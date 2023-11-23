#ifndef SW_MATRIX_HXX
#define SW_MATRIX_HXX
#pragma once

#include "glm/glm.hpp"

struct vec2;
namespace Matrix {
glm::mat4 CalculateTransform(vec2 position, float rotation, vec2 scale, const glm::mat4 &base = glm::mat4(1.f));
bool DecomposeTransform(const glm::mat4 &mat, vec2 *position, float *rotation, vec2 *scale);
} // namespace Matrix

#endif // SW_MATRIX_HXX