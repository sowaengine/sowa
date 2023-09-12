#ifndef SW_GUI_HXX
#define SW_GUI_HXX
#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>

#include "math/math.hxx"

struct GuiColor {
	float r;
	float g;
	float b;
	float a;

	GuiColor() = default;

	GuiColor(float r, float g, float b, float a = 1.f) {
		this->r = r;
		this->g = g;
		this->b = b;
		this->a = a;
	}

	static GuiColor RGBA255(int r, int g, int b, int a = 255) {
		GuiColor color;
		color.r = r / 255.f;
		color.g = g / 255.f;
		color.b = b / 255.f;
		color.a = a / 255.f;
		return color;
	}
};

class Gui {
  public:
	using ID = std::string;

	void BeginWindow(const std::string &title);
	void EndWindow();

	void Text(const std::string &text);

	void PutWindowSize(int width, int height);

	void Update();

	bool state_lmouse_just_pressed = false;
	bool state_lmouse_down = false;
	vec2 state_mouse_pos = vec2(0.f);
	vec2 state_mouse_motion = vec2(0.f);

  private:
	struct WindowData {
		ID id;
		std::string title;
		float posX;
		float posY;
		float width;
		float height;
		float cursorX; // in pixels
		float cursorY; // in pixels
		bool active;
	};

	bool has_window(ID id);
	WindowData &get_window_data(ID id);
	float px_to_percentage_w(float px);
	float px_to_percentage_h(float px);
	float percentage_to_px_w(float percentage);
	float percentage_to_px_h(float percentage);

	int m_windowWidth = 0;
	int m_windowHeight = 0;
	ID m_currentWindow;

	enum class ActiveWindowOperation {
		Move = 0,
		Resize
	};
	ID m_activeWindow;
	ActiveWindowOperation m_activeWindowOperation;

	float m_stateTitleHeight = 24.f;
	GuiColor m_windowBgColor = GuiColor::RGBA255(20, 20, 20);
	GuiColor m_windowTitleColor = GuiColor::RGBA255(37, 126, 204);
	float m_configTextPadding = 2.f;

	std::unordered_map<ID, WindowData> m_windows;
};

#endif // SW_GUI_HXX