#ifndef SW_CAMERA_2D_HPP_
#define SW_CAMERA_2D_HPP_

#include "node2d.hpp"
#include "math/math.hpp"

namespace sowa {
class Camera2D : public Node2D {
  public:
	static std::string Typename() { return "sowa::Camera2D"; }

	Camera2D();
	~Camera2D();

    inline sowa::vec2f& Zoom() { return m_Zoom; }

  private:
    sowa::vec2f m_Zoom = {1.f, 1.f};
};
} // namespace sowa

#endif // SW_CAMERA_2D_HPP_