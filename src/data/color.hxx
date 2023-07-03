#ifndef SW_COLOR_HXX
#define SW_COLOR_HXX
#pragma once

#include <stdint.h>

struct Color {
	static Color FromRGB(uint8_t r, uint8_t g, uint8_t b) {
		Color c;
		c.r = r / 255.f;
		c.g = g / 255.f;
		c.b = b / 255.f;
		c.a = 1.f;
		return c;
	}

	static Color FromRGBFloat(float r, float g, float b) {
		Color c;
		c.r = r;
		c.g = g;
		c.b = b;
		c.a = 1.f;
		return c;
	}

	float r = 1.f;
	float g = 1.f;
	float b = 1.f;
	float a = 1.f;
};

#endif // SW_COLOR_HXX