#include "text.hxx"

#include "core/app.hxx"

void Text::_update() {
	App::get().Renderer().DrawText(m_text, &App::get().TestFont(), m_position.x, m_position.y, 0.f, glm::mat4(1.f), 0.f, 1.f);
}