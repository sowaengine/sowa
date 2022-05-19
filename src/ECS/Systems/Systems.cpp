#include "Systems.h"
#include "ECS/Scene/Scene.h"

namespace Ease::Systems
{
   void ProcessAll(Ease::Scene* pScene)
   {
      Systems::System_SpriteRenderer2D(pScene);
      Systems::System_AnimatedSprite2D(pScene);
      Systems::System_TextRenderer2D(pScene);
   }
} // namespace Ease::Systems
