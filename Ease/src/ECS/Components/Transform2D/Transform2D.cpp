#include "ECS/Components/Transform2D/Transform2D.hpp"

namespace Ease::Component
{
   Transform2D::Transform2D()
      : m_Position(0.f, 0.f), m_Scale(1.f, 1.f), m_Rotation(0.f), m_ZIndex(0)
   {

   }

   Transform2D::~Transform2D()
   {

   }
}