#ifndef SW_MOUSE_BUTTON_EVENT_HXX
#define SW_MOUSE_BUTTON_EVENT_HXX
#pragma once

#include "core/input.hxx"

struct InputEventMouseButton {
	MouseButton button = MB_NONE;
	ClickAction action = NONE;
	InputModifiers modifiers;
};

#endif // SW_MOUSE_BUTTON_EVENT_HXX