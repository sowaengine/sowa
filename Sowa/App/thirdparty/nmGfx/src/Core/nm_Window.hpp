#ifndef __NM_GFX_WINDOW_HPP__
#define __NM_GFX_WINDOW_HPP__
#pragma once

#include <stdint.h>

class GLFWwindow;
namespace nmGfx
{
    enum WindowFlags : uint32_t
    {
        NONE = 0,
        FULLSCREEN = 1 << 1,
    };

    // pure virtual base class that has window methods
    class Window
    {
    public:
        // virtual void CreateWindow(int width, int height, const char* title, WindowFlags flags) = 0;
        bool ShouldClose();
        
        void PollEvents();
        void SwapBuffers();

        // void SetClearColor(float r, float g, float b, float a = 1.0f);
        // void Clear(uint32_t bits = ClearBit::COLOR);

        ~Window() = default;
        Window(int windowWidth, int windowHeight, int videoWidth, int videoHeight, const char* title, WindowFlags flags);
        Window() = default;

        void SetFullscreen(bool fullscreen);
        inline bool GetFullscreen() { return _fullscreen; }

        // Returns x mouse pos relative to window
        int GetWindowMousePosX();
        // Returns y mouse pos relative to window
        int GetWindowMousePosY();

        // Returns x mouse pos relative to video space
        int GetVideoMousePosX();
        // Returns y mouse pos relative to window
        int GetVideoMousePosY();


        int GetWindowHeight();
        int GetWindowWidth();

        inline int GetVideoWidth() { return _videoWidth; }
        inline int GetVideoHeight() { return _videoHeight; }

        void UnbindFramebuffer();

        
        inline GLFWwindow* GetGLFWwindow() { return _pWindow; }


        /* Input */
        enum class MouseButton
        {
            LEFT = 0,
            MIDDLE,
            RIGHT,
        };
        enum class MousePressState
        {
            CLICKED = 0,
            RELEASED,
        };
        enum class CursorMode
        {
            NORMAL = 0,
            HIDDEN,
            DISABLED,
        };
		enum ModifierKeys : unsigned int {
			ModifierKeys_NONE = 0,
			ModifierKeys_MOD_CONTROL = 1 << 1,
			ModifierKeys_MOD_SHIFT = 2 << 1,
			ModifierKeys_MOD_ALT = 3 << 1,
			ModifierKeys_MOD_SUPER = 4 << 1,
			ModifierKeys_MOD_CAPS_LOCK = 5 << 1,
			ModifierKeys_MOD_NUM_LOCK = 6 << 1,
		};
		// maps one-to-one to glfw keys
		enum class Key : int {
			// glfw3.h
			Key_UNKNOWN = -1,

			Key_SPACE = 32,
			Key_APOSTROPHE = 39, /* ' */
			Key_COMMA = 44,		 /* , */
			Key_MINUS = 45,		 /* - */
			Key_PERIOD = 46,	 /* . */
			Key_SLASH = 47,		 /* / */
			Key_0 = 48,
			Key_1 = 49,
			Key_2 = 50,
			Key_3 = 51,
			Key_4 = 52,
			Key_5 = 53,
			Key_6 = 54,
			Key_7 = 55,
			Key_8 = 56,
			Key_9 = 57,
			Key_SEMICOLON = 59, /* ; */
			Key_EQUAL = 61,		/* = */
			Key_A = 65,
			Key_B = 66,
			Key_C = 67,
			Key_D = 68,
			Key_E = 69,
			Key_F = 70,
			Key_G = 71,
			Key_H = 72,
			Key_I = 73,
			Key_J = 74,
			Key_K = 75,
			Key_L = 76,
			Key_M = 77,
			Key_N = 78,
			Key_O = 79,
			Key_P = 80,
			Key_Q = 81,
			Key_R = 82,
			Key_S = 83,
			Key_T = 84,
			Key_U = 85,
			Key_V = 86,
			Key_W = 87,
			Key_X = 88,
			Key_Y = 89,
			Key_Z = 90,
			Key_LEFT_BRACKET = 91,	/* [ */
			Key_BACKSLASH = 92,		/* \ */
			Key_RIGHT_BRACKET = 93, /* ] */
			Key_GRAVE_ACCENT = 96,	/* ` */
			Key_WORLD_1 = 161,		/* non-US #1 */
			Key_WORLD_2 = 162,		/* non-US #2 */

			/* Function keys */
			Key_ESCAPE = 256,
			Key_ENTER = 257,
			Key_TAB = 258,
			Key_BACKSPACE = 259,
			Key_INSERT = 260,
			Key_Key_DELETE = 261,
			Key_RIGHT = 262,
			Key_LEFT = 263,
			Key_DOWN = 264,
			Key_UP = 265,
			Key_PAGE_UP = 266,
			Key_PAGE_DOWN = 267,
			Key_HOME = 268,
			Key_END = 269,
			Key_CAPS_LOCK = 280,
			Key_SCROLL_LOCK = 281,
			Key_NUM_LOCK = 282,
			Key_PRINT_SCREEN = 283,
			Key_PAUSE = 284,
			Key_F1 = 290,
			Key_F2 = 291,
			Key_F3 = 292,
			Key_F4 = 293,
			Key_F5 = 294,
			Key_F6 = 295,
			Key_F7 = 296,
			Key_F8 = 297,
			Key_F9 = 298,
			Key_F10 = 299,
			Key_F11 = 300,
			Key_F12 = 301,
			Key_F13 = 302,
			Key_F14 = 303,
			Key_F15 = 304,
			Key_F16 = 305,
			Key_F17 = 306,
			Key_F18 = 307,
			Key_F19 = 308,
			Key_F20 = 309,
			Key_F21 = 310,
			Key_F22 = 311,
			Key_F23 = 312,
			Key_F24 = 313,
			Key_F25 = 314,
			Key_KP_0 = 320,
			Key_KP_1 = 321,
			Key_KP_2 = 322,
			Key_KP_3 = 323,
			Key_KP_4 = 324,
			Key_KP_5 = 325,
			Key_KP_6 = 326,
			Key_KP_7 = 327,
			Key_KP_8 = 328,
			Key_KP_9 = 329,
			Key_KP_DECIMAL = 330,
			Key_KP_DIVIDE = 331,
			Key_KP_MULTIPLY = 332,
			Key_KP_SUBTRACT = 333,
			Key_KP_ADD = 334,
			Key_KP_ENTER = 335,
			Key_KP_EQUAL = 336,
			Key_LEFT_SHIFT = 340,
			Key_LEFT_CONTROL = 341,
			Key_LEFT_ALT = 342,
			Key_LEFT_SUPER = 343,
			Key_RIGHT_SHIFT = 344,
			Key_RIGHT_CONTROL = 345,
			Key_RIGHT_ALT = 346,
			Key_RIGHT_SUPER = 347,
			Key_MENU = 348,
		};

		bool IsMouseButtonPressed(Window::MouseButton button);
		void SetCursorMode(Window::CursorMode mode);
		bool IsWindowHovered();
		bool IsKeyPressed(Window::Key key);

	  private:
		friend class Renderer;

		GLFWwindow *_pWindow = nullptr;

		int _videoWidth;
        int _videoHeight;

        // Stores window values after setting fullscreen to revert on returning back to windowed mode
        bool _fullscreen = false;
        int _fullscreen_oldX = 0;
        int _fullscreen_oldY = 0;
        int _fullscreen_oldW = 0;
        int _fullscreen_oldH = 0;
        int _fullscreen_oldRefresh = 0;
    };
    
} // namespace nmGfx


#endif // __NM_GFX_WINDOW_HPP__