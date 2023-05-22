#ifndef SW_RECT_HPP_
#define SW_RECT_HPP_

#include "vec2.hpp"

namespace sowa {
struct rect {
  public:
	float x = 0.f;
	float y = 0.f;
	float w = 0.f;
	float h = 0.f;

	rect();
	rect(float w_, float h_);
	rect(float x_, float y_, float w_, float h_);

    /**
     * @brief maps point in 'this' to point in 'target'
    */
    vec2 mapPoint(const vec2& point, const rect& target);
};
} // namespace sowa

#endif