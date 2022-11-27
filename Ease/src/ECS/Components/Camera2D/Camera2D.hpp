#ifndef _E_CAMERA2D_HPP__
#define _E_CAMERA2D_HPP__

#pragma once

#include <glm/glm.hpp>

#include "Utils/Math.hpp"

namespace Ease::Component {
class Camera2D {
  public:
	Camera2D();
	~Camera2D();

	inline bool &Current() { return _Current; }
	inline bool &Rotatable() { return _Rotatable; }
	inline float &Zoom() { return _Zoom; }

	inline Vec2 &Center() { return _Center; }

  public:
	bool _Current = false;
	bool _Rotatable = true;
	float _Zoom = 1.f;

	Vec2 _Center{0.5f, 0.5f};
};
} // namespace Ease::Component

#endif