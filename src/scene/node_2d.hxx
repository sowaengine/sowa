#ifndef SW_NODE_2D_HXX
#define SW_NODE_2D_HXX
#pragma once

#include "math/math.hxx"
#include "node.hxx"

#include "glm/glm.hpp"

class Node2D : public Node {
  public:
	virtual ~Node2D() = default;

	inline vec2 &Position() { return m_position; }
	inline vec2 &Scale() { return m_scale; }
	inline float &Rotation() { return m_rotation; }
	inline float &ZIndex() { return m_zIndex; }

	glm::mat4 CalculateTransform();

	vec2 GlobalPosition();
	float GlobalRotation();

  protected:
	vec2 m_position = vec2(0.f, 0.f);
	vec2 m_scale = vec2(1.f, 1.f);
	float m_zIndex = 0.f;
	float m_rotation = 0.f;
};

#endif // SW_NODE_2D_HXX