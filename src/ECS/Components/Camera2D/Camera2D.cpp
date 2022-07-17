#include "Camera2D.hpp"

namespace Ease::Component
{
   Camera2D::Camera2D()
      : m_Current(false)
   {
      m_Camera.zoom = 1.f;
   }

   Camera2D::~Camera2D()
   {

   }
}