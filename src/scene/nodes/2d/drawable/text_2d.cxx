#include "text_2d.hxx"

#include "core/app.hxx"

void Text2D::_update() {
	if (!is_visible())
		return;

	App::get().Renderer().draw_text(m_text, &App::get().TestFont(), 0.f, 0.f, z_index(), calculate_transform(), id(), 1.f, 0.f, 0.f, m_color_red, m_color_green, m_color_blue, m_color_alpha);
}