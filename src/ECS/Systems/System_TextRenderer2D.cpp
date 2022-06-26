#include "Systems.hpp"
#include "ECS/Scene/Scene.hpp"

#include "ECS/Components/Transform2D/Transform2D.hpp"
#include "ECS/Components/TextRenderer2D/TextRenderer2D.hpp"

#include "Core/Renderer.hpp"

namespace Ease::Systems
{
   void System_TextRenderer2D(Ease::Scene* pScene)
   {
      auto view = pScene->m_Registry.view<Component::Transform2D, Component::TextRenderer2D>();
      for(const auto& entityID : view)
      {
         Entity entity(entityID, &pScene->m_Registry);
         auto& transformc = entity.GetComponent<Component::Transform2D>();
         auto& textrendererc = entity.GetComponent<Component::TextRenderer2D>();

         if(!textrendererc.Visible()) continue;

         Renderer::get_singleton().DrawText(transformc.Position(), transformc.Scale(), transformc.ZIndex(), transformc.Rotation(), textrendererc.FontSize(), textrendererc.Text(), textrendererc.Color());
      }
   }
} // namespace Ease::Systems
