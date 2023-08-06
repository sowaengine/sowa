#include "text.hxx"

#include "core/app.hxx"

void Text::Update() {
	App::GetInstance().Renderer().DrawText(m_text, &App::GetInstance().TestFont(), m_position.x, m_position.y, glm::mat4(1.f), 0.f, 1.f);
}