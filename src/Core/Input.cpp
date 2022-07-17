#include "Input.hpp"
#include "raylib.h"
#include "Ease.hpp"
#include "Application.hpp"

typedef Vector2 rVec2;

namespace Ease
{
   Vec2 Input::GetMousePos()
   {
      Ease::Window& window = Ease::Application::get_singleton().GetWindow();

      rVec2 mousePos = GetMousePosition();
      //mousePos.x -= GetScreenWidth() * 0.5f;
      //mousePos.y -= GetScreenHeight() * 0.5f;
#ifndef EASE_EDITOR
      mousePos.x -= window.GetBlackbarWidth();
      mousePos.y -= window.GetBlackbarHeight();

#else
      Ease::Vec2 scale;
      scale.x = (float)window.GetWindowWidth() / (float)window.Editor_GetWindowWidth();
      scale.y = (float)window.GetWindowHeight() / (float)window.Editor_GetWindowHeight();
      
      mousePos.x -= window.Editor_GetWindowPosX();
      mousePos.y -= window.Editor_GetWindowPosY();
      mousePos.x *= scale.x;
      mousePos.y *= scale.y;
#endif

      return Ease::Vec2(mousePos.x, mousePos.y);
   }

   Vec2 Input::GetGameMousePos()
   {
      Ease::Window& window = Ease::Application::get_singleton().GetWindow();
      
      Ease::Vec2 scale;
#ifndef EASE_EDITOR
      scale.x = (float)window.GetVideoWidth() / ((float)window.GetWindowWidth() - (window.GetBlackbarWidth() * 2));
      scale.y = (float)window.GetVideoHeight() / ((float)window.GetWindowHeight() - (window.GetBlackbarHeight() * 2));
#else
      scale.x = (float)window.GetVideoWidth() / (float)window.GetWindowWidth();
      scale.y = (float)window.GetVideoHeight() / (float)window.GetWindowHeight();
#endif

      Ease::Vec2 mousePos = Input::GetMousePos();
      mousePos.x *= scale.x;
      mousePos.y *= scale.y;

      return mousePos;
   }

   bool Input::IsMouseButtonDown(Button button)
   {
      int btn = 
            button == Button::LEFT ? MOUSE_BUTTON_LEFT
            : button == Button::MIDDLE ? MOUSE_BUTTON_MIDDLE
            : button == Button::RIGHT ? MOUSE_BUTTON_RIGHT
            : MOUSE_BUTTON_LEFT;
      
      return ::IsMouseButtonDown(btn);
   }
   bool Input::IsMouseButtonClicked(Button button)
   {
      int btn = 
            button == Button::LEFT ? MOUSE_BUTTON_LEFT
            : button == Button::MIDDLE ? MOUSE_BUTTON_MIDDLE
            : button == Button::RIGHT ? MOUSE_BUTTON_RIGHT
            : MOUSE_BUTTON_LEFT;
      
      return ::IsMouseButtonPressed(btn);
   }
   bool Input::IsMouseButtonReleased(Button button)
   {
      int btn = 
            button == Button::LEFT ? MOUSE_BUTTON_LEFT
            : button == Button::MIDDLE ? MOUSE_BUTTON_MIDDLE
            : button == Button::RIGHT ? MOUSE_BUTTON_RIGHT
            : MOUSE_BUTTON_LEFT;
      
      return ::IsMouseButtonReleased(btn);
   }
} // namespace Ease
