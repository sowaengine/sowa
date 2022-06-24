#include "AnimatedSprite2D.hpp"
#include "Debug.hpp"

namespace Ease::Component
{
   AnimatedSprite2D::AnimatedSprite2D()
      : m_CurrentFrame(0), m_SelectedAnimation(""), m_FPS(1), m_TicksPast(0.f)
   {

   }

   AnimatedSprite2D::~AnimatedSprite2D()
   {

   }
   
   void AnimatedSprite2D::Step(float ms)
   {
      if(m_SelectedAnimation == "") return;
      
      m_TicksPast += ms;
      if(m_TicksPast > 1.f / m_FPS)
      {
         m_TicksPast -= 1.f / m_FPS;
         m_CurrentFrame++;

         m_CurrentFrame %= GetAnimation(m_SelectedAnimation).frameCount;
      }
   }
}