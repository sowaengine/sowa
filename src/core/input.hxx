#ifndef SW_INPUT_HXX
#define SW_INPUT_HXX
#pragma once

#include "servers/input_server.hxx"

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

typedef int KeyAction;
static const KeyAction KEY_PRESSED = 0;
static const KeyAction KEY_RELEASED = 1;
static const KeyAction KEY_REPEAT = 2;

typedef int Key;
static const Key KEY_UNKNOWN = -1;
static const Key KEY_SPACE = 32;
static const Key KEY_APOSTROPHE = 39; /* ' */
static const Key KEY_COMMA = 44;	  /* , */
static const Key KEY_MINUS = 45;	  /* - */
static const Key KEY_PERIOD = 46;	  /* . */
static const Key KEY_SLASH = 47;	  /* / */
static const Key KEY_0 = 48;
static const Key KEY_1 = 49;
static const Key KEY_2 = 50;
static const Key KEY_3 = 51;
static const Key KEY_4 = 52;
static const Key KEY_5 = 53;
static const Key KEY_6 = 54;
static const Key KEY_7 = 55;
static const Key KEY_8 = 56;
static const Key KEY_9 = 57;
static const Key KEY_SEMICOLON = 59; /* ; */
static const Key KEY_EQUAL = 61;	 /* = */
static const Key KEY_A = 65;
static const Key KEY_B = 66;
static const Key KEY_C = 67;
static const Key KEY_D = 68;
static const Key KEY_E = 69;
static const Key KEY_F = 70;
static const Key KEY_G = 71;
static const Key KEY_H = 72;
static const Key KEY_I = 73;
static const Key KEY_J = 74;
static const Key KEY_K = 75;
static const Key KEY_L = 76;
static const Key KEY_M = 77;
static const Key KEY_N = 78;
static const Key KEY_O = 79;
static const Key KEY_P = 80;
static const Key KEY_Q = 81;
static const Key KEY_R = 82;
static const Key KEY_S = 83;
static const Key KEY_T = 84;
static const Key KEY_U = 85;
static const Key KEY_V = 86;
static const Key KEY_W = 87;
static const Key KEY_X = 88;
static const Key KEY_Y = 89;
static const Key KEY_Z = 90;
static const Key KEY_LEFT_BRACKET = 91;	 /* [ */
static const Key KEY_BACKSLASH = 92;	 /* \ */
static const Key KEY_RIGHT_BRACKET = 93; /* ] */
static const Key KEY_GRAVE_ACCENT = 96;	 /* ` */
static const Key KEY_WORLD_1 = 161;		 /* non-US #1 */
static const Key KEY_WORLD_2 = 162;		 /* non-US #2 */

static const Key KEY_ESCAPE = 256;
static const Key KEY_ENTER = 257;
static const Key KEY_TAB = 258;
static const Key KEY_BACKSPACE = 259;
static const Key KEY_INSERT = 260;
static const Key KEY_DELETE = 261;
static const Key KEY_RIGHT = 262;
static const Key KEY_LEFT = 263;
static const Key KEY_DOWN = 264;
static const Key KEY_UP = 265;
static const Key KEY_PAGE_UP = 266;
static const Key KEY_PAGE_DOWN = 267;
static const Key KEY_HOME = 268;
static const Key KEY_END = 269;
static const Key KEY_CAPS_LOCK = 280;
static const Key KEY_SCROLL_LOCK = 281;
static const Key KEY_NUM_LOCK = 282;
static const Key KEY_PRINT_SCREEN = 283;
static const Key KEY_PAUSE = 284;
static const Key KEY_F1 = 290;
static const Key KEY_F2 = 291;
static const Key KEY_F3 = 292;
static const Key KEY_F4 = 293;
static const Key KEY_F5 = 294;
static const Key KEY_F6 = 295;
static const Key KEY_F7 = 296;
static const Key KEY_F8 = 297;
static const Key KEY_F9 = 298;
static const Key KEY_F10 = 299;
static const Key KEY_F11 = 300;
static const Key KEY_F12 = 301;
static const Key KEY_F13 = 302;
static const Key KEY_F14 = 303;
static const Key KEY_F15 = 304;
static const Key KEY_F16 = 305;
static const Key KEY_F17 = 306;
static const Key KEY_F18 = 307;
static const Key KEY_F19 = 308;
static const Key KEY_F20 = 309;
static const Key KEY_F21 = 310;
static const Key KEY_F22 = 311;
static const Key KEY_F23 = 312;
static const Key KEY_F24 = 313;
static const Key KEY_F25 = 314;
static const Key KEY_KP_0 = 320;
static const Key KEY_KP_1 = 321;
static const Key KEY_KP_2 = 322;
static const Key KEY_KP_3 = 323;
static const Key KEY_KP_4 = 324;
static const Key KEY_KP_5 = 325;
static const Key KEY_KP_6 = 326;
static const Key KEY_KP_7 = 327;
static const Key KEY_KP_8 = 328;
static const Key KEY_KP_9 = 329;
static const Key KEY_KP_DECIMAL = 330;
static const Key KEY_KP_DIVIDE = 331;
static const Key KEY_KP_MULTIPLY = 332;
static const Key KEY_KP_SUBTRACT = 333;
static const Key KEY_KP_ADD = 334;
static const Key KEY_KP_ENTER = 335;
static const Key KEY_KP_EQUAL = 336;
static const Key KEY_LEFT_SHIFT = 340;
static const Key KEY_LEFT_CONTROL = 341;
static const Key KEY_LEFT_ALT = 342;
static const Key KEY_LEFT_SUPER = 343;
static const Key KEY_RIGHT_SHIFT = 344;
static const Key KEY_RIGHT_CONTROL = 345;
static const Key KEY_RIGHT_ALT = 346;
static const Key KEY_RIGHT_SUPER = 347;
static const Key KEY_MENU = 348;

class Input {
  public:
	static void GetMousePosition(double &x, double &y);
	static void GetWindowMousePosition(double &x, double &y);
	static int GetPressedChar();

	static ActionState GetKeyState(int key);
	static bool IsKeyDown(int key);
	static bool IsKeyUp(int key);
	static bool IsKeyJustPressed(int key);
	static bool IsKeyJustReleased(int key);

	static ActionState GetButtonState(int button);
	static bool IsButtonDown(int button);
	static bool IsButtonUp(int button);
	static bool IsButtonJustPressed(int button);
	static bool IsButtonJustReleased(int button);

	static bool IsButtonJustClicked(int button);
	static bool IsButtonJustDoubleClicked(int button);
};

#endif // SW_INPUT_HXX