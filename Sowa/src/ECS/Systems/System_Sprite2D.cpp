#include "ECS/Scene/Scene.hpp"
#include "ECS/Systems/Systems.hpp"

#include "ECS/Components/Sprite2D/Sprite2D.hpp"
#include "ECS/Components/Transform2D/Transform2D.hpp"

#include "Core/Application.hpp"
#include "Core/Renderer.hpp"
#include "Resource/ResourceManager.hpp"
#include "Resource/Texture/Texture.hpp"

namespace Sowa::Systems {
void System_Sprite2D(Sowa::Scene *pScene, bool pickable /* = false*/) {
	ResourceManager<Sowa::ImageTexture> &loader = pScene->GetResourceManager<Sowa::ImageTexture>();

	auto view = pScene->m_Registry.view<Component::Transform2D, Component::Sprite2D>();
	for (const auto &entityID : view) {
		Entity entity(entityID, &pScene->m_Registry);
		auto &transformc = entity.GetComponent<Component::Transform2D>();
		auto &spritec = entity.GetComponent<Component::Sprite2D>();

		if (!spritec.Visible())
			continue;

		Reference<Sowa::ImageTexture> tex = spritec.Texture();

		if (tex == nullptr)
			continue;

		Renderer::get_singleton().DrawTexture(
			transformc.Position(),
			{transformc.Scale().x * (tex->Width()),
			 transformc.Scale().y * (tex->Height())},
			transformc.ZIndex(),
			transformc.Rotation(),
			*tex,
			(uint32_t)entityID);
	}
}
} // namespace Sowa::Systems
