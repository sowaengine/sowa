#include "sprite_2d.hxx"

#include "core/app.hxx"

#include <iostream>

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include "resource/resource_manager.hxx"

void Sprite2D::Update() {

	Texture *texture = ResourceManager::GetInstance().GetAs<Texture>(m_texture);
	if (nullptr == texture)
		return;

	uint32_t textureID = texture->ID();

	glm::mat4 model(1.f);
	model = glm::translate(model, {m_position.x, m_position.y, 0.f});
	model = glm::rotate(model, glm::radians(m_rotation), {0.f, 0.f, 1.f});
	model = glm::scale(model, {m_scale.x, m_scale.y, 1.f});
	model = glm::scale(model, {texture->Width(), texture->Height(), 1.f});

	glm::vec4 points[4] = {
		{-0.5f, 0.5f, 0.f, 1.f},
		{-0.5f, -0.5f, 0.f, 1.f},
		{0.5f, -0.5f, 0.f, 1.f},
		{0.5f, 0.5f, 0.f, 1.f}};
	glm::vec2 uvs[4] = {
		{0.f, 1.f},
		{0.f, 0.f},
		{1.f, 0.f},
		{1.f, 1.f}};

	BatchVertex vertices[4];
	for (int i = 0; i < 4; i++) {
		points[i] = model * points[i];

		vertices[i].x = points[i].x;
		vertices[i].y = points[i].y;
		vertices[i].z = m_zIndex;
		vertices[i].r = 1.f;
		vertices[i].g = 1.f;
		vertices[i].b = 1.f;
		vertices[i].a = 1.f;
		vertices[i].u = uvs[i].x;
		vertices[i].v = uvs[i].y;
		vertices[i].t_id = textureID;
		vertices[i].d_id = 0.f;
		vertices[i].draw_mode = 0.f;
	}

	App::GetInstance().Renderer().PushQuad(vertices);
}