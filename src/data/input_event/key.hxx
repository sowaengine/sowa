#ifndef SW_KEY_EVENT_HXX
#define SW_KEY_EVENT_HXX
#pragma once

#include "core/input.hxx"

struct InputEventKey {
	Key key;
	KeyAction action = NONE;
	InputModifiers modifiers;
};

#endif // SW_KEY_EVENT_HXX