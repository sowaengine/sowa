#ifndef SW_INPUT_SERVER_HXX
#define SW_INPUT_SERVER_HXX
#pragma once

#include <unordered_map>

#include "core/rendering/gl.hxx"

enum class ActionState {
	NONE,
	DOWN,
	UP,
	JUST_PRESSED,
	JUST_RELEASED
};

class InputServer {
  public:
	InputServer();
	static InputServer &GetInstance();

	void ProcessInput();
	void PollEvents();

	void GetMousePosition(double &x, double &y);
	void GetWindowMousePosition(double &x, double &y);
	int GetPressedChar();

	ActionState GetKeyState(int key);
	bool IsKeyDown(int key);
	bool IsKeyUp(int key);
	bool IsKeyJustPressed(int key);
	bool IsKeyJustReleased(int key);

	ActionState GetButtonState(int button);
	bool IsButtonDown(int button);
	bool IsButtonUp(int button);
	bool IsButtonJustPressed(int button);
	bool IsButtonJustReleased(int button);

	bool IsButtonJustClicked(int button);
	bool IsButtonJustDoubleClicked(int button);

  private:
	friend struct InputCallbackBridge;
	friend class RenderingServer;

	std::unordered_map<int, ActionState> m_keyStates;
	std::unordered_map<int, ActionState> m_buttonStates;

	std::unordered_map<int, bool> m_buttonSingleClicked;
	std::unordered_map<int, bool> m_buttonDoubleClicked;

	/// @brief Stores currently active character input (now keydown) (gets value from character event)
	int m_pressedChar = 0;

	double m_input_mouseX = 0.f;
	double m_input_mouseY = 0.f;

	void mouse_button_callback(GLFWwindow *window, int button, int action, int pressed);
	void cursor_pos_callback(GLFWwindow *window, double x, double y);
	void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);
	void char_callback(GLFWwindow *window, unsigned int codepoint);
	void scroll_callback(GLFWwindow *window, double xOffset, double yOffset);

	void initialize();
};

#endif // SW_INPUT_SERVER_HXX