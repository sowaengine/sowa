#ifndef SW_MOUSE_MOVE_EVENT_HXX
#define SW_MOUSE_MOVE_EVENT_HXX
#pragma once

struct InputEventMouseMove {
	float mouseX = 0.f;
	float mouseY = 0.f;
	float deltaX = 0.f;
	float deltaY = 0.f;
};

#endif // SW_MOUSE_MOVE_EVENT_HXX