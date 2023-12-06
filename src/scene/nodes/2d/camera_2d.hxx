#ifndef SW_CAMERA_2D_HXX
#define SW_CAMERA_2D_HXX
#pragma once

#include "math/math.hxx"
#include "node_2d.hxx"

class Camera2D : public Node2D {
  public:
	virtual ~Camera2D() = default;

	void _update() override;

	vec2 &zoom() { return m_zoom; }
	vec2 &center_point() { return m_center_point; }
	bool &rotatable() { return m_rotatable; }

	rect get_bounds();

  private:
	vec2 m_zoom = vec2(1.f, 1.f);
	vec2 m_center_point = vec2(0.5f, 0.5f);
	bool m_rotatable = false;
};

#endif // SW_CAMERA_2D_HXX