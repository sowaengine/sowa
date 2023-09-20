#ifndef SW_NODE_2D_HXX
#define SW_NODE_2D_HXX
#pragma once

#include "math/math.hxx"
#include "node.hxx"

#include "glm/glm.hpp"

class Node2D : public Node {
  public:
	virtual ~Node2D() = default;

	inline vec2 &position() { return m_position; }
	inline vec2 &scale() { return m_scale; }
	inline float &rotation() { return m_rotation; }
	inline float &z_index() { return m_zindex; }

	glm::mat4 calculate_transform();
	glm::mat4 calculate_local_transform();

	vec2 global_position();
	float global_rotation();
	vec2 global_scale();

	void set_global_transform(const glm::mat4 &transform);

	bool decompose_transform(vec2 *position, float *rotation, vec2 *scale);

  private:
	glm::mat4 get_parent_transform();

  protected:
	vec2 m_position = vec2(0.f, 0.f);
	vec2 m_scale = vec2(1.f, 1.f);
	float m_zindex = 0.f;
	float m_rotation = 0.f;
};

#endif // SW_NODE_2D_HXX