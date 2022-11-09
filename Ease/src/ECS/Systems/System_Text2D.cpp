#include "ECS/Scene/Scene.hpp"
#include "ECS/Systems/Systems.hpp"

#include "ECS/Components/Text2D/Text2D.hpp"
#include "ECS/Components/Transform2D/Transform2D.hpp"

#include "Core/Renderer.hpp"

namespace Ease::Systems {
void System_Text2D(Ease::Scene *pScene, bool pickable /* = false*/) {
	auto view = pScene->m_Registry.view<Component::Transform2D, Component::Text2D>();
	for (const auto &entityID : view) {
		Entity entity(entityID, &pScene->m_Registry);
		auto &transformc = entity.GetComponent<Component::Transform2D>();
		auto &textrendererc = entity.GetComponent<Component::Text2D>();

		if (!textrendererc.Visible())
			continue;

		// if(pickable)
		//    Renderer::get_singleton().DrawTextWithID(transformc.Position(), transformc.Scale(), transformc.ZIndex(), transformc.Rotation(), textrendererc.FontSize(), textrendererc.Text(), textrendererc.Color(), (uint32_t)entityID);
		// else
		//    Renderer::get_singleton().DrawText(transformc.Position(), transformc.Scale(), transformc.ZIndex(), transformc.Rotation(), textrendererc.FontSize(), textrendererc.Text(), textrendererc.Color());
	}
}
} // namespace Ease::Systems
