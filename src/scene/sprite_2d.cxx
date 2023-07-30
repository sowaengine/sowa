#include "sprite_2d.hxx"

#include "core/app.hxx"

#include <iostream>

void Sprite2D::Update() {
	if (nullptr == m_texture)
		return;

	uint32_t textureID = m_texture->ID();

	App::GetInstance().Renderer().PushQuad(m_position.x, m_position.y, 0.f, m_texture->Width(), m_texture->Height(), 1.f, 1.f, 1.f, 1.f, 0.f, static_cast<float>(textureID), 0.f);
}