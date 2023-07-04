#ifndef SW_MARGIN_HXX
#define SW_MARGIN_HXX
#pragma once

struct LTRBRect {
	static LTRBRect LTRB(float left, float top, float right, float bottom) {
		LTRBRect m;

		m.left = left;
		m.top = top;
		m.right = right;
		m.bottom = bottom;

		return m;
	}

	static LTRBRect All(float all) {
		LTRBRect m;

		m.left = all;
		m.top = all;
		m.right = all;
		m.bottom = all;

		return m;
	}

	float left = 0.f;
	float top = 0.f;
	float right = 0.f;
	float bottom = 0.f;
};

typedef LTRBRect Margin;
typedef LTRBRect Padding;

#endif // SW_MARGIN_HXX