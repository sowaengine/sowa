#include "ECS/Components/AnimatedSprite2D/AnimatedSprite2D.hpp"
#include "Debug.hpp"
#include "Resource/ResourceManager.hpp"
#include "Resource/SpriteSheetAnimation/SpriteSheetAnimation.hpp"

namespace Ease::Component
{
   AnimatedSprite2D::AnimatedSprite2D()
      : m_CurrentFrame(0), m_SelectedAnimation(0), m_SelectedAnimationName(""), m_FPS(1), m_TicksPast(0.f)
   {

   }

   AnimatedSprite2D::~AnimatedSprite2D()
   {

   }
   
   void AnimatedSprite2D::Step(float ms)
   {
      if(m_SelectedAnimationName == "") return;

      static auto loader = ResourceManager<SpriteSheetAnimation>::GetLoader();
      std::shared_ptr<SpriteSheetAnimation> anim = loader.GetResource(m_SelectedAnimation);
      
      m_TicksPast += ms;
      if(m_TicksPast > 1.f / m_FPS)
      {
         m_TicksPast -= 1.f / m_FPS;
         m_CurrentFrame++;

         m_CurrentFrame %= anim->FrameCount();
      }
   }

   void AnimatedSprite2D::SetAnimation(const std::string& name, ResourceID animation)
   {
      m_Animations[name] = animation;
   }

   void AnimatedSprite2D::SelectAnimation(const std::string& name)
   {
      static auto loader = ResourceManager<SpriteSheetAnimation>::GetLoader();

      if(m_Animations.count(name) == 0) return;
      if(!loader.HasResource(m_Animations[name])) return;

      m_SelectedAnimationName = name;
      m_SelectedAnimation = m_Animations[name];
      
      std::shared_ptr<SpriteSheetAnimation> anim = loader.GetResource(m_SelectedAnimation);

      m_FPS = anim->FPS();
   }

   ResourceID AnimatedSprite2D::GetCurrentTexture()
   {
      static auto loader = ResourceManager<SpriteSheetAnimation>::GetLoader();

      if(loader.HasResource(m_SelectedAnimation))
         return loader.GetResource(m_SelectedAnimation)->Texture();
      return 0;
   }
}