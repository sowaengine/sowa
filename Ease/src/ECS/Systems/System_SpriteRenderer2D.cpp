#include "ECS/Systems/Systems.hpp"
#include "ECS/Scene/Scene.hpp"

#include "ECS/Components/Transform2D/Transform2D.hpp"
#include "ECS/Components/SpriteRenderer2D/SpriteRenderer2D.hpp"

#include "Core/Renderer.hpp"
#include "Resource/ResourceManager.hpp"
#include "Resource/Texture/Texture.hpp"

namespace Ease::Systems
{
   void System_SpriteRenderer2D(Ease::Scene* pScene, bool pickable /* = false*/)
   {
      ResourceManager<Ease::Texture>& loader = ResourceManager<Ease::Texture>::GetLoader();
      

      auto view = pScene->m_Registry.view<Component::Transform2D, Component::SpriteRenderer2D>();
      for(const auto& entityID : view)
      {
         Entity entity(entityID, &pScene->m_Registry);
         auto& transformc = entity.GetComponent<Component::Transform2D>();
         auto& spritec = entity.GetComponent<Component::SpriteRenderer2D>();
         
         if(!loader.HasResource(spritec.TextureID())) continue;
         if(!spritec.Visible()) continue;
         
         if(pickable)
            Renderer::get_singleton().DrawQuadWithID(transformc.Position(), transformc.Scale(), transformc.ZIndex(), transformc.Rotation(), *loader.GetResource(spritec.TextureID()).get(), (uint32_t)entityID, glm::vec2(0.f, 0.f), glm::vec2(1.f, 1.f));
         else
            Renderer::get_singleton().DrawQuad(transformc.Position(), transformc.Scale(), transformc.ZIndex(), transformc.Rotation(), *loader.GetResource(spritec.TextureID()).get(), glm::vec2(0.f, 0.f), glm::vec2(1.f, 1.f));
      }
   }
} // namespace Ease::Systems
