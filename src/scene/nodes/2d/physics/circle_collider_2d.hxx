#ifndef SW_CIRCLE_COLLIDER_2D_HXX
#define SW_CIRCLE_COLLIDER_2D_HXX
#pragma once

#include "base_collider_2d.hxx"

class CircleCollider2D : public BaseCollider2D {
  public:
	inline float &radius() { return f0; }

	void *body_add_fixture(void *body) override;

	void _update() override;

  private:
};

#endif // SW_CIRCLE_COLLIDER_2D_HXX