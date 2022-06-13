#include "Systems.h"
#include "ECS/Scene/Scene.h"

namespace Ease::Systems
{
   void ProcessAll(Ease::Scene* pScene, SystemsFlags flags)
   {
      if(flags & SystemsFlags::SpriteRenderer2D)
         Systems::System_SpriteRenderer2D(pScene);

      if(flags & SystemsFlags::AnimatedSprite2D)
         Systems::System_AnimatedSprite2D(pScene);
         
      if(flags & SystemsFlags::TextRenderer2D)
         Systems::System_TextRenderer2D(pScene);
   }
} // namespace Ease::Systems
