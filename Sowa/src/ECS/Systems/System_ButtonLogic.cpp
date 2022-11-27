#include "ECS/Scene/Scene.hpp"
#include "ECS/Systems/Systems.hpp"

#include "ECS/Components/Button/Button.hpp"
#include "ECS/Components/UITransform/UITransform.hpp"

#include "Core/Application.hpp"
#include "Core/Input.hpp"

#include <iostream>

namespace Sowa::Systems {
void System_ButtonLogic(Sowa::Scene *pScene) {
	Sowa::Application &app = Sowa::Application::get_singleton();
	Sowa::Entity pickedEntity = app.SelectedEntity();

	auto view = app.GetCurrentScene()->m_Registry.view<Component::Button>();
	for (auto &e : view) {
		Entity entity(e, &app.GetCurrentScene()->m_Registry);
		auto &buttonc = entity.GetComponent<Component::Button>();

		Component::Button::ButtonState &state = buttonc.Internal_GetState();
		// if(state == Component::Button::ButtonState::CLICK && entity == pickedEntity && Input::IsMouseButtonDown(Sowa::Input::Button::LEFT))
		//{
		//
		//}
		// else
		//{
		//   state = Component::Button::ButtonState::NORMAL;
		//}
	}

	if (pickedEntity.HasComponent<Sowa::Component::UITransform>() && pickedEntity.HasComponent<Sowa::Component::Button>()) {
		auto &transformc = pickedEntity.GetComponent<Sowa::Component::UITransform>();
		auto &buttonc = pickedEntity.GetComponent<Sowa::Component::Button>();

		if (buttonc.Disabled() && buttonc.GetState() != Sowa::Component::Button::ButtonState::DISABLED)
			buttonc.Internal_GetState() = Sowa::Component::Button::ButtonState::DISABLED;
		else if (buttonc.GetState() == Sowa::Component::Button::ButtonState::DISABLED)
			buttonc.Internal_GetState() = Sowa::Component::Button::ButtonState::NORMAL;

		// if(buttonc.GetState() != Sowa::Component::Button::ButtonState::DISABLED && Input::IsMouseButtonClicked(Sowa::Input::Button::LEFT))
		// {
		//    buttonc.Internal_GetState() = Sowa::Component::Button::ButtonState::CLICK;
		//    buttonc.OnClick.Invoke();
		// }

		if (buttonc.GetState() != Sowa::Component::Button::ButtonState::CLICK && buttonc.GetState() != Sowa::Component::Button::ButtonState::DISABLED)
			buttonc.Internal_GetState() = Sowa::Component::Button::ButtonState::HOVER;
	}
}
} // namespace Sowa::Systems
