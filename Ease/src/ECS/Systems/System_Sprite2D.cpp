#include "ECS/Scene/Scene.hpp"
#include "ECS/Systems/Systems.hpp"

#include "ECS/Components/Sprite2D/Sprite2D.hpp"
#include "ECS/Components/Transform2D/Transform2D.hpp"

#include "Core/Application.hpp"
#include "Core/Renderer.hpp"
#include "Resource/ResourceManager.hpp"
#include "Resource/Texture/Texture.hpp"

namespace Ease::Systems {
void System_Sprite2D(Ease::Scene *pScene, bool pickable /* = false*/) {
	ResourceManager<Ease::ImageTexture> &loader = pScene->GetResourceManager<Ease::ImageTexture>();

	auto view = pScene->m_Registry.view<Component::Transform2D, Component::Sprite2D>();
	for (const auto &entityID : view) {
		Entity entity(entityID, &pScene->m_Registry);
		auto &transformc = entity.GetComponent<Component::Transform2D>();
		auto &spritec = entity.GetComponent<Component::Sprite2D>();

		if (!spritec.Visible())
			continue;

		Reference<Ease::ImageTexture> tex = spritec.Texture();

		if (tex == nullptr)
			continue;

		Renderer::get_singleton().DrawTexture(
			transformc.Position(),
			{transformc.Scale().x * (tex->Width()),
			 transformc.Scale().y * (tex->Height())},
			transformc.ZIndex(),
			transformc.Rotation(),
			*tex,
			pickable ? (uint32_t)entityID : 0);
	}
}
} // namespace Ease::Systems
