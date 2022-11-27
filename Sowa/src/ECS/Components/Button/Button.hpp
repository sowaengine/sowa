#ifndef _E_BUTTON_HPP__
#define _E_BUTTON_HPP__

#pragma once
#include "ECS/Components/NinePatchRect/NinePatchRect.hpp"
#include "Resource/Texture/Texture.hpp"
#include "Utils/Event.hpp"
#include <memory>
#include <stdint.h>

typedef void(*ButtonClickEvent);

namespace Sowa::Component {
class Button {
  public:
	enum class ButtonState {
		NORMAL = 0,
		HOVER,
		CLICK,
		DISABLED
	};

	Button();
	~Button();

	Sowa::Component::NinePatchRect &TextureNormal() { return m_TextureNormal; }
	Sowa::Component::NinePatchRect &TextureHover() { return m_TextureHover; }
	Sowa::Component::NinePatchRect &TextureClick() { return m_TextureClick; }
	Sowa::Component::NinePatchRect &TextureDisabled() { return m_TextureDisabled; }

	std::string &Text() { return m_Text; }

	bool &Visible() { return m_Visible; }
	bool &Disabled() { return m_Disabled; }

	ButtonState GetState() { return m_ButtonState; }

	ButtonState &Internal_GetState() { return m_ButtonState; }

	Sowa::Event<void()> OnClick;

	static Sowa::Component::NinePatchRect GetDefaultButtonTexture(ButtonState state);

  private:
	Sowa::Component::NinePatchRect m_TextureNormal;
	Sowa::Component::NinePatchRect m_TextureHover;
	Sowa::Component::NinePatchRect m_TextureClick;
	Sowa::Component::NinePatchRect m_TextureDisabled;

	std::string m_Text{"Button"};

	bool m_Visible = true;
	bool m_Disabled = false;

	ButtonState m_ButtonState = ButtonState::NORMAL;
};
} // namespace Sowa::Component

#endif