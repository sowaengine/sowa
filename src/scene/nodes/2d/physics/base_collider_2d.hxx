#ifndef SW_BASE_COLLIDER_2D_HXX
#define SW_BASE_COLLIDER_2D_HXX
#pragma once

#include "scene/nodes/2d/node_2d.hxx"

class BaseCollider2D : public Node2D {
  public:
	virtual void *body_add_fixture(void *body) = 0;

	inline void *get_fixture() { return m_fixture; };

  protected:
	float f0; // width
	float f1; // height

  private:
	void *m_fixture = nullptr;
};

#endif // SW_BASE_COLLIDER_2D_HXX