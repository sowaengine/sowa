#ifndef _E_BUTTON_HPP__
#define _E_BUTTON_HPP__

#pragma once
#include <stdint.h>
#include <memory>
#include "Resource/Texture/Texture.hpp"
#include "ECS/Components/NinePatchRect/NinePatchRect.hpp"
#include "Utils/Event.hpp"

typedef void(*ButtonClickEvent);

namespace Ease::Component
{
    class Button
    {
    public:
        enum class ButtonState
        {
            NORMAL = 0,
            HOVER,
            CLICK,
            DISABLED
        };

        Button();
        ~Button();

        Ease::Component::NinePatchRect& TextureNormal() { return m_TextureNormal; }
        Ease::Component::NinePatchRect& TextureHover() { return m_TextureHover; }
        Ease::Component::NinePatchRect& TextureClick() { return m_TextureClick; }
        Ease::Component::NinePatchRect& TextureDisabled() { return m_TextureDisabled; }
        
        std::string& Text() { return m_Text; }

        bool& Visible() { return m_Visible; }
        bool& Disabled() { return m_Disabled; }

        ButtonState GetState() { return m_ButtonState; }



        ButtonState& Internal_GetState() { return m_ButtonState; }

        Ease::Event<void()> OnClick;

        static Ease::Component::NinePatchRect GetDefaultButtonTexture(ButtonState state);
    private:
        Ease::Component::NinePatchRect m_TextureNormal;
        Ease::Component::NinePatchRect m_TextureHover;
        Ease::Component::NinePatchRect m_TextureClick;
        Ease::Component::NinePatchRect m_TextureDisabled;

        std::string m_Text{"Button"};

        bool m_Visible = true;
        bool m_Disabled = false;

        ButtonState m_ButtonState = ButtonState::NORMAL;
    };
}

#endif