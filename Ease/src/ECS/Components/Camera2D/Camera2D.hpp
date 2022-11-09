#ifndef _E_CAMERA2D_HPP__
#define _E_CAMERA2D_HPP__

#pragma once

#include <glm/glm.hpp>
// #include "raylib.h"
#pragma warning("Camera Broken")

namespace Ease::Component {
class Camera2D {
  public:
	Camera2D();
	~Camera2D();

	float &Zoom() {
		static float x = 0;
		return x;
	} // m_Camera.zoom; }

	bool &Current() { return m_Current; }

  private:
	bool m_Current = false;
};
} // namespace Ease::Component

#endif