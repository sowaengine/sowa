#include "gui.hxx"

#include "core/app.hxx"

void Gui::BeginWindow(const std::string &title) {
	if (!has_window(title)) {
		WindowData data;
		data.id = title;
		data.title = title;
		data.posX = 0;
		data.posY = 70;
		data.width = 30;
		data.height = 30;
		m_windows[title] = data;
	}

	WindowData &data = get_window_data(title);
	data.active = true;
	data.cursorX = 0.f;
	data.cursorY = m_stateTitleHeight;
	m_currentWindow = title;

	float x = percentage_to_px_w(data.posX);
	float y = percentage_to_px_h(data.posY);
	float w = percentage_to_px_w(data.width);
	float h = percentage_to_px_h(data.height);

	App::GetInstance().Renderer().PushQuad(x, y + h - m_stateTitleHeight, 1.f, w, m_stateTitleHeight, m_windowTitleColor.r, m_windowTitleColor.g, m_windowTitleColor.b, 1.f, 0.f, 0.f);
	App::GetInstance().Renderer().PushQuad(x, y, 1.f, w, h - m_stateTitleHeight, m_windowBgColor.r, m_windowBgColor.g, m_windowBgColor.b, 1.f, 0.f, 0.f);

	vec2 textSize = App::GetInstance().TestFont().CalcTextSize(data.title);
	textSize *= 0.5f;
	float yBegin = y + h - m_stateTitleHeight;
	yBegin += (textSize.y - 8.f) * 0.5f;
	App::GetInstance().Renderer().DrawText(data.title, &App::GetInstance().TestFont(), x + 4.f, yBegin, 1.f, glm::mat4(1.f), 0.f, 0.4f);
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
	vec2 textSize = App::GetInstance().TestFont().CalcTextSize(text) * 0.4f;
	textSize.y += m_configTextPadding * 2;

	App::GetInstance()
		.Renderer()
		.DrawText(text, &App::GetInstance().TestFont(), percentage_to_px_w(window.posX) + window.cursorX + m_configTextPadding, percentage_to_px_h(window.posY) + percentage_to_px_h(window.height) - window.cursorY - textSize.y + m_configTextPadding, 1.f, glm::mat4(1.f), 0.f, 0.4f);

	window.cursorX = 0.f;
	window.cursorY += textSize.y;
}

void Gui::Update() {
	for (auto &[id, data] : m_windows) {
		if (!data.active)
			continue;

		data.active = false;
		float x = percentage_to_px_w(data.posX);
		float y = percentage_to_px_h(data.posY);
		float w = percentage_to_px_w(data.width);
		float h = percentage_to_px_h(data.height);

		if (state_lmouse_just_pressed && rect(x, y + h - m_stateTitleHeight, w, m_stateTitleHeight).is_point_in(state_mouse_pos)) {
			m_activeWindow = data.id;
			m_activeWindowOperation = ActiveWindowOperation::Move;
		}
		if (state_lmouse_just_pressed && rect(x + w - 16.f, y - 16.f, 32.f, 32.f).is_point_in(state_mouse_pos)) {
			m_activeWindow = data.id;
			m_activeWindowOperation = ActiveWindowOperation::Resize;
		}
	}

	if (!state_lmouse_down && m_activeWindow != ID()) {
		m_activeWindow = ID();
	}

	if (m_activeWindow != ID() && has_window(m_activeWindow)) {
		WindowData &window = get_window_data(m_activeWindow);
		if (m_activeWindowOperation == ActiveWindowOperation::Move) {
			window.posX += px_to_percentage_w(state_mouse_motion.x * 2.f);
			window.posY -= px_to_percentage_h(state_mouse_motion.y * 2.f);
		} else if (m_activeWindowOperation == ActiveWindowOperation::Resize) {
			float h = px_to_percentage_h(state_mouse_motion.y * 2.f);
			window.width += px_to_percentage_w(state_mouse_motion.x * 2.f);
			window.posY -= h;
			window.height += h;
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