#ifndef SW_RECT_HXX
#define SW_RECT_HXX
#pragma once

class vec2;

class rect {
  public:
	float x = 0.f;
	float y = 0.f;
	float w = 0.f;
	float h = 0.f;

	rect() = default;
	rect(float x_, float y_) : x(x_), y(y_) {}
	rect(float x_, float y_, float w_, float h_) : x(x_), y(y_), w(w_), h(h_) {}
	~rect() = default;

	/**
	 * @brief maps point in 'this' to point in 'target'
	 */
	vec2 map_point(const vec2 &point, const rect &target);

  private:
};

#endif // SW_RECT_HXX