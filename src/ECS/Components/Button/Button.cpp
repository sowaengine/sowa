#include "Button.hpp"
#include "Resource/ResourceManager.hpp"
#include "Resource/Texture/Texture.hpp"
#include <iostream>

namespace Ease::Component
{
   Button::Button()
   {   
      m_TextureNormal = GetDefaultButtonTexture(ButtonState::NORMAL);
      m_TextureHover = GetDefaultButtonTexture(ButtonState::HOVER);
      m_TextureClick = GetDefaultButtonTexture(ButtonState::CLICK);
      m_TextureDisabled = GetDefaultButtonTexture(ButtonState::DISABLED);
   }

   Button::~Button()
   {

   }

   // static
   Ease::Component::NinePatchRect Button::GetDefaultButtonTexture(ButtonState state)
   {
      static Ease::Component::NinePatchRect s_NormalRect;
      static Ease::Component::NinePatchRect s_HoverRect;
      static Ease::Component::NinePatchRect s_DisabledRect;
      static Ease::Component::NinePatchRect s_ClickedRect;
      static auto& tex_loader = Ease::ResourceManager<Ease::Texture>::GetLoader();
      static bool first = true;
      if(first)
      {
         s_NormalRect.Texture() = tex_loader.LoadResource("abs://res/button_white.png", EASE_DEFAULT_RESOURCE__TEXTURE2D_BUTTON_NORMAL);
         s_NormalRect.Left() = s_NormalRect.Right() = s_NormalRect.Top() = s_NormalRect.Bottom() = 40;

         s_HoverRect.Texture() = tex_loader.LoadResource("abs://res/button_dark.png", EASE_DEFAULT_RESOURCE__TEXTURE2D_BUTTON_HOVER);
         s_HoverRect.Left() = s_HoverRect.Right() = s_HoverRect.Top() = s_HoverRect.Bottom() = 40;

         s_DisabledRect.Texture() = tex_loader.LoadResource("abs://res/button_white.png", EASE_DEFAULT_RESOURCE__TEXTURE2D_BUTTON_DISABLED);
         s_DisabledRect.Left() = s_DisabledRect.Right() = s_DisabledRect.Top() = s_DisabledRect.Bottom() = 40;

         s_ClickedRect.Texture() = tex_loader.LoadResource("abs://res/button_white.png", EASE_DEFAULT_RESOURCE__TEXTURE2D_BUTTON_CLICKED);
         s_ClickedRect.Left() = s_ClickedRect.Right() = s_ClickedRect.Top() = s_ClickedRect.Bottom() = 40;

         first = false;
      }

      switch (state)
      {
      case ButtonState::NORMAL:
         return s_NormalRect;
         break;
      case ButtonState::HOVER:
         return s_HoverRect;
         break;
      case ButtonState::DISABLED:
         return s_DisabledRect;
         break;
      case ButtonState::CLICK:
         return s_ClickedRect;
         break;
      default:
         break;
      }
      return s_NormalRect;
   }
}