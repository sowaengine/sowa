#include "Systems.h"
#include "ECS/Scene/Scene.h"

#include "ECS/Components/Transform2D/Transform2D.h"
#include "ECS/Components/AnimatedSprite2D/AnimatedSprite2D.h"

#include "Core/Renderer.h"
#include "Resource/ResourceManager.h"
#include "Resource/Texture/Texture.h"

namespace Ease::Systems
{
   void System_AnimatedSprite2D(Ease::Scene* pScene)
   {
      ResourceManager<Ease::Texture>& loader = ResourceManager<Ease::Texture>::GetLoader();
      

      auto view = pScene->m_Registry.view<Component::Transform2D, Component::AnimatedSprite2D>();
      for(const auto& entityID : view)
      {
         Entity entity(entityID, &pScene->m_Registry);
         auto& transformc = entity.GetComponent<Component::Transform2D>();
         auto& animsprc = entity.GetComponent<Component::AnimatedSprite2D>();
         
         if(animsprc.SelectedAnimation() == "" || !loader.HasResource(animsprc.GetCurrentTexture())) continue;

         animsprc.Step(GetFrameTime());

         glm::vec2 uv1;
         glm::vec2 uv2;

         Ease::Texture tex = *loader.GetResource(animsprc.GetCurrentTexture()).get();
         Ease::Animation anim = animsprc.GetAnimation(animsprc.SelectedAnimation());
         
         glm::vec2 size = { 1.0f / anim.hFrames, 1.0f / anim.vFrames };
         glm::vec2 pos = { anim.startFrame + animsprc.CurrentFrame() * size.x, anim.SelectedRow * size.y };

         uv1 = pos;
         uv2 = pos + size;

         Renderer::get_singleton().DrawQuad(transformc.Position(), transformc.Scale(), transformc.ZIndex(), transformc.Rotation(), tex, uv1, uv2);
      }
   }
} // namespace Ease::Systems
