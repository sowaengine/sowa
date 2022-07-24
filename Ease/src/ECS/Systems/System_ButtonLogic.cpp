#include "ECS/Systems/Systems.hpp"
#include "ECS/Scene/Scene.hpp"

#include "ECS/Components/UITransform/UITransform.hpp"
#include "ECS/Components/Button/Button.hpp"

#include "Core/Application.hpp"
#include "Core/Input.hpp"

#include <iostream>

namespace Ease::Systems
{
   void System_ButtonLogic(Ease::Scene* pScene)
   {
      Ease::Application& app = Ease::Application::get_singleton();
      Ease::Entity pickedEntity = app.GetPickedEntity();

      auto view = app.GetCurrentScene()->m_Registry.view<Component::Button>();
      for(auto& e : view)
      {
         Entity entity(e, &app.GetCurrentScene()->m_Registry);
         auto& buttonc = entity.GetComponent<Component::Button>();

         Component::Button::ButtonState& state = buttonc.Internal_GetState();
         if(state == Component::Button::ButtonState::CLICK && entity == pickedEntity && Input::IsMouseButtonDown(Ease::Input::Button::LEFT))
         {

         }
         else
         {
            state = Component::Button::ButtonState::NORMAL;
         }
      }

      if(pickedEntity.HasComponent<Ease::Component::UITransform>() && pickedEntity.HasComponent<Ease::Component::Button>())
      {
         auto& transformc = pickedEntity.GetComponent<Ease::Component::UITransform>();
         auto& buttonc = pickedEntity.GetComponent<Ease::Component::Button>();

         if(buttonc.Disabled() && buttonc.GetState() != Ease::Component::Button::ButtonState::DISABLED)
            buttonc.Internal_GetState() = Ease::Component::Button::ButtonState::DISABLED;
         else if(buttonc.GetState() == Ease::Component::Button::ButtonState::DISABLED)
            buttonc.Internal_GetState() = Ease::Component::Button::ButtonState::NORMAL;


         if(buttonc.GetState() != Ease::Component::Button::ButtonState::DISABLED && Input::IsMouseButtonClicked(Ease::Input::Button::LEFT))
         {
            buttonc.Internal_GetState() = Ease::Component::Button::ButtonState::CLICK;
            buttonc.OnClick.Invoke();
         }
         
         if(buttonc.GetState() != Ease::Component::Button::ButtonState::CLICK && buttonc.GetState() != Ease::Component::Button::ButtonState::DISABLED)
            buttonc.Internal_GetState() = Ease::Component::Button::ButtonState::HOVER;
      }
   }
} // namespace Ease::Systems
