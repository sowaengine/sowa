#ifndef SW_MATH_HXX
#define SW_MATH_HXX
#pragma once

#include "matrix.hxx"
#include "rect.hxx"
#include "vec2.hxx"

namespace math {

float sin(float f);
float cos(float f);
float pi();
float radians(float deg);
float degrees(float rad);
float atan2(float y, float x);
float fmod(float x, float y);
float sqrt(float v);

}; // namespace math

#endif // SW_MATH_HXX