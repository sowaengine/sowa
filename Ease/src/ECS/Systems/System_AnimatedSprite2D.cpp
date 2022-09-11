#include "ECS/Scene/Scene.hpp"
#include "ECS/Systems/Systems.hpp"

#include "ECS/Components/AnimatedSprite2D/AnimatedSprite2D.hpp"
#include "ECS/Components/Transform2D/Transform2D.hpp"

#include "Core/Renderer.hpp"
#include "Resource/ResourceManager.hpp"
#include "Resource/SpriteSheetAnimation/SpriteSheetAnimation.hpp"
#include "Resource/Texture/Texture.hpp"

namespace Ease::Systems {
void System_AnimatedSprite2D(Ease::Scene *pScene, bool pickable /* = false*/) {
	ResourceManager<Ease::Texture> &tex_loader = pScene->GetResourceManager<Ease::Texture>();
	ResourceManager<Ease::SpriteSheetAnimation> &anim_loader = pScene->GetResourceManager<Ease::SpriteSheetAnimation>();

	auto view = pScene->m_Registry.view<Component::Transform2D, Component::AnimatedSprite2D>();
	for (const auto &entityID : view) {
		Entity entity(entityID, &pScene->m_Registry);
		auto &transformc = entity.GetComponent<Component::Transform2D>();
		auto &animsprc = entity.GetComponent<Component::AnimatedSprite2D>();

		if (animsprc.GetSelectedAnimationName() == "") {
			if (animsprc.Animations().size() == 0)
				continue;

			auto it = animsprc.Animations().begin();
			// std::advance(it, 0);
			animsprc.SelectAnimation(it->first);
		}
		if (!tex_loader.HasResource(animsprc.GetCurrentTexture()))
			continue;

		// animsprc.Step(GetFrameTime());

		glm::vec2 uv1;
		glm::vec2 uv2;

		Ease::Texture tex = *tex_loader.GetResource(animsprc.GetCurrentTexture()).get();
		std::shared_ptr<Ease::SpriteSheetAnimation> anim = anim_loader.GetResource(animsprc.GetSelectedAnimation());

		glm::vec2 size = {1.0f / anim->HFrames(), 1.0f / anim->VFrames()};
		glm::vec2 pos = {anim->StartFrame() + animsprc.CurrentFrame() * size.x, anim->SelectedRow() * size.y};

		uv1 = pos;
		uv2 = pos + size;

		// if(pickable)
		//    Renderer::get_singleton().DrawQuadWithID(transformc.Position(), transformc.Scale(), transformc.ZIndex(), transformc.Rotation(), tex, (uint32_t)entityID, uv1, uv2);
		// else
		//    Renderer::get_singleton().DrawQuad(transformc.Position(), transformc.Scale(), transformc.ZIndex(), transformc.Rotation(), tex, uv1, uv2);
	}
}
} // namespace Ease::Systems
