#ifndef SW_RECT_COLLIDER_2D_HXX
#define SW_RECT_COLLIDER_2D_HXX
#pragma once

#include "base_collider_2d.hxx"

class RectCollider2D : public BaseCollider2D {
  public:
	inline float &width() { return f0; }
	inline float &height() { return f1; }

	void *body_add_fixture(void *body) override;

	void _update() override;

  private:
};

#endif // SW_RECT_COLLIDER_2D_HXX