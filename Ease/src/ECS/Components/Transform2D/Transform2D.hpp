#ifndef _E_TRANSFORM2D_HPP__
#define _E_TRANSFORM2D_HPP__

#pragma once

#include "Utils/Math.hpp"

namespace Sowa::Component {
class Transform2D {
  public:
	Transform2D();
	~Transform2D();

	Vec2 &Position() { return m_Position; }
	Vec2 &Scale() { return m_Scale; }
	float &Rotation() { return m_Rotation; }
	int &ZIndex() { return m_ZIndex; }

  public:
	Vec2 m_Position;
	Vec2 m_Scale;
	float m_Rotation;
	int m_ZIndex;
};
} // namespace Sowa::Component

#endif