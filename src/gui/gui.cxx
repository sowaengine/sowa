#include "gui.hxx"

#include "core/app.hxx"

void Gui::BeginWindow(const std::string &title, GuiWindowFlags flags) {
	if (!has_window(title)) {
		WindowData data;
		data.id = title;
		data.title = title;
		data.posX = 0;
		data.posY = 70;
		data.width = 30;
		data.height = 30;
		data.flags = flags;
		m_windows[title] = data;
	}

	WindowData &data = get_window_data(title);
	data.active = true;
	data.cursorX = 0.f;
	data.cursorY = m_stateTitleHeight;
	m_currentWindow = title;

	data.width = std::max(1.f, data.width);
	data.height = std::max(1.f + px_to_percentage_h(m_stateTitleHeight), data.height);

	float x = percentage_to_px_w(data.posX);
	float y = percentage_to_px_h(data.posY);
	float w = percentage_to_px_w(data.width);
	float h = percentage_to_px_h(data.height);

	App::get().Renderer().PushQuad(x, y + h - m_stateTitleHeight, 1.f, w, m_stateTitleHeight, m_windowTitleColor.r, m_windowTitleColor.g, m_windowTitleColor.b, 1.f, 0.f, 0.f);
	App::get().Renderer().PushQuad(x, y, 1.f, w, h - m_stateTitleHeight, m_windowBgColor.r, m_windowBgColor.g, m_windowBgColor.b, 1.f, 0.f, 0.f);

	vec2 textSize = App::get().TestFont().CalcTextSize(data.title);
	textSize *= 0.5f;
	float textY = y + h - m_stateTitleHeight;
	float textPadding = m_stateTitleHeight * 0.33f;
	textY = y + h - (m_stateTitleHeight) + ((m_stateTitleHeight - textSize.y) * 1.f);
	float textX = x + textPadding;
	if (data.flags & GuiWindowFlag_TitleCentered) {
		textX = x + (w * 0.5f) - (textSize.x * 0.5f);
	}
	App::get().Renderer().draw_text(data.title, &App::get().TestFont(), textX, textY, 1.f, glm::mat4(1.f), 0.f, 0.4f, 0.f, 0.f, 1.f, 1.f, 1.f, 1.f, calculate_bounds(data));
}
void Gui::EndWindow() {
	m_currentWindow = ID();
}

void Gui::PutWindowSize(int width, int height) {
	m_windowWidth = width;
	m_windowHeight = height;
}

void Gui::Text(const std::string &text) {
	if (!has_window(m_currentWindow)) {
		return;
	}

	WindowData &window = get_window_data(m_currentWindow);
	vec2 textSize = App::get().TestFont().CalcTextSize(text) * 0.4f;
	textSize.y += m_configTextPadding * 2;

	App::get()
		.Renderer()
		.draw_text(text, &App::get().TestFont(), percentage_to_px_w(window.posX) + window.cursorX + m_configTextPadding, percentage_to_px_h(window.posY) + percentage_to_px_h(window.height) - window.cursorY - textSize.y + m_configTextPadding, 1.f, glm::mat4(1.f), 0.f, 0.4f, 0.f, 0.f, 1.f, 1.f, 1.f, 1.f, calculate_bounds(window));

	window.cursorX = 0.f;
	window.cursorY += textSize.y;
}

void Gui::Update() {
	m_cursorState = GuiCursorState::None;
	if (!state_lmouse_down) {
		m_activeWindowOperation = ActiveWindowOperation::None;
	}

	for (auto &[id, data] : m_windows) {
		if (!data.active)
			continue;

		if (!state_lmouse_down) {
			if (data.posY + data.height > 100.f) {
				data.posY = 100.f - data.height;
			}
			if (data.posY + data.height < 3.f) {
				data.posY = 5.f - data.height;
			}
			if (data.posX > 97.f) {
				data.posX = 95.f;
			}
			if (data.posX + data.width < 3.f) {
				data.posX = 5.f - data.width;
			}
		}

		data.active = false;
		float x = percentage_to_px_w(data.posX);
		float y = percentage_to_px_h(data.posY);
		float w = percentage_to_px_w(data.width);
		float h = percentage_to_px_h(data.height);

		m_possibleWindowOperation = ActiveWindowOperation::None;
		GuiCursorState possibleCursorState = GuiCursorState::None;

		if (m_activeWindowOperation == ActiveWindowOperation::None) {

			if (rect(x, y + h - m_stateTitleHeight, w, m_stateTitleHeight).is_point_in(state_mouse_pos)) {
				m_possibleWindowOperation = ActiveWindowOperation::Move;
			}

			// bottom
			if (rect(x + 16.f, y - 16.f, w - 32.f, 32.f).is_point_in(state_mouse_pos)) {
				m_possibleWindowOperation = ActiveWindowOperation::Resize_Y;
				m_activeWindowResize_Move_X = false;
				possibleCursorState = GuiCursorState::Resize_Y;
			}
			// left
			if (rect(x - 16.f, y + 16.f, 32.f, h - 32.f).is_point_in(state_mouse_pos)) {
				m_possibleWindowOperation = ActiveWindowOperation::Resize_X;
				m_activeWindowResize_Move_X = true;
				possibleCursorState = GuiCursorState::Resize_X;
			}
			// right
			if (rect(x + w - 16.f, y + 16.f, 32.f, h - 32.f).is_point_in(state_mouse_pos)) {
				m_possibleWindowOperation = ActiveWindowOperation::Resize_X;
				m_activeWindowResize_Move_X = false;
				possibleCursorState = GuiCursorState::Resize_X;
			}
			// bottom right
			if (rect(x + w - 16.f, y - 16.f, 32.f, 32.f).is_point_in(state_mouse_pos)) {
				m_possibleWindowOperation = ActiveWindowOperation::Resize;
				m_activeWindowResize_Move_X = false;
				possibleCursorState = GuiCursorState::Resize;
			}
			// bottom left
			if (rect(x - 16.f, y - 16.f, 32.f, 32.f).is_point_in(state_mouse_pos)) {
				m_possibleWindowOperation = ActiveWindowOperation::Resize;
				m_activeWindowResize_Move_X = true;
				possibleCursorState = GuiCursorState::Resize;
			}
		}

		if (possibleCursorState != GuiCursorState::None) {
			m_cursorState = possibleCursorState;
		}

		if (state_lmouse_just_pressed && m_possibleWindowOperation != ActiveWindowOperation::None && m_activeWindowOperation == ActiveWindowOperation::None) {
			m_activeWindow = data.id;
			m_activeWindowOperation = m_possibleWindowOperation;
		}
	}

	if (!state_lmouse_down && m_activeWindow != ID()) {
		m_activeWindow = ID();
	}

	if (m_activeWindow != ID() && has_window(m_activeWindow)) {
		WindowData &window = get_window_data(m_activeWindow);
		if (m_activeWindowOperation == ActiveWindowOperation::Move) {
			if (window.flags & GuiWindowFlag_Movable) {
				window.posX += px_to_percentage_w(state_mouse_motion.x * 2.f);
				window.posY -= px_to_percentage_h(state_mouse_motion.y * 2.f);
			}
		} else if (m_activeWindowOperation == ActiveWindowOperation::Resize || m_activeWindowOperation == ActiveWindowOperation::Resize_X || m_activeWindowOperation == ActiveWindowOperation::Resize_Y) {
			float w = px_to_percentage_w(state_mouse_motion.x * 2.f);
			float h = px_to_percentage_h(state_mouse_motion.y * 2.f);

			if (window.flags & GuiWindowFlag_ResizableX && m_activeWindowOperation != ActiveWindowOperation::Resize_Y) {
				if (m_activeWindowResize_Move_X) {
					window.width -= w;
					if (window.width > 1.f) {
						window.posX += w;
					}
				} else {
					window.width += w;
				}
			}
			if (window.flags & GuiWindowFlag_ResizableY && m_activeWindowOperation != ActiveWindowOperation::Resize_X) {
				window.height += h;
				if (window.height > 1.f + px_to_percentage_h(m_stateTitleHeight)) {
					window.posY -= h;
				}
			}
		}
	}
}

bool Gui::has_window(ID id) {
	return m_windows.find(id) != m_windows.end();
}

Gui::WindowData &Gui::get_window_data(ID id) {
	return m_windows[id];
}

float Gui::px_to_percentage_w(float px) {
	return px / (float)m_windowWidth * 100.f;
}

float Gui::px_to_percentage_h(float px) {
	return px / (float)m_windowHeight * 100.f;
}

float Gui::percentage_to_px_w(float percentage) {
	return (float)m_windowWidth / 100.f * percentage;
}

float Gui::percentage_to_px_h(float percentage) {
	return (float)m_windowHeight / 100.f * percentage;
}

rect Gui::calculate_bounds(const WindowData &window) {
	return rect(percentage_to_px_w(window.posX), percentage_to_px_h(window.posY), percentage_to_px_w(window.width), percentage_to_px_h(window.height));
}