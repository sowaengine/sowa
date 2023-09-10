#ifndef SW_CLICK_EVENT_HXX
#define SW_CLICK_EVENT_HXX
#pragma once

#include "core/input.hxx"

struct InputEventClick {
	MouseButton button;
	bool doubleClick;
};

#endif // SW_CLICK_EVENT_HXX