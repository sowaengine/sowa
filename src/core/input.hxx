#ifndef SW_INPUT_HXX
#define SW_INPUT_HXX
#pragma once

static const int NONE = -1;

typedef int MouseButton;
static const MouseButton MB_NONE = -1;
static const MouseButton MB_LEFT = 0;
static const MouseButton MB_RIGHT = 1;
static const MouseButton MB_MIDDLE = 2;
static const MouseButton MB_1 = 0;
static const MouseButton MB_2 = 1;
static const MouseButton MB_3 = 2;
static const MouseButton MB_4 = 3;
static const MouseButton MB_5 = 4;
static const MouseButton MB_6 = 5;
static const MouseButton MB_7 = 6;
static const MouseButton MB_8 = 7;

typedef int ClickAction;
static const ClickAction PRESSED = 0;
static const ClickAction RELEASED = 1;

struct InputModifiers {
	bool shift = false;
	bool control = false;
	bool alt = false;
	bool super = false;
};

#endif // SW_INPUT_HXX