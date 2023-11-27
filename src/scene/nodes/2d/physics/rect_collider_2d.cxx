#include "rect_collider_2d.hxx"

#include "core/app.hxx"
#include "servers/physics_server_2d.hxx"
#include "utils/utils.hxx"

#include "glm/gtc/matrix_transform.hpp"

void *RectCollider2D::body_add_fixture(void *body) {
	return PhysicsServer2D::get().body_add_box_shape(this, body, id(), vec2(width() * 0.5f, height() * 0.5f), position(), rotation());
}

void RectCollider2D::_update() {

	glm::mat4 model = calculate_transform();
	model = glm::scale(model, {width(), height(), 1.f});

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
		vertices[i].z = m_zindex;
		vertices[i].r = 30.f / 255.f;
		vertices[i].g = 190.f / 255.f;
		vertices[i].b = 230.f / 255.f;
		vertices[i].a = 1.f;
		vertices[i].u = uvs[i].x;
		vertices[i].v = uvs[i].y;
		vertices[i].t_id = 0.f;
		vertices[i].d_id = static_cast<float>(id());
		vertices[i].draw_mode = 3.f;
	}

	App::get().Renderer().PushQuad(vertices);
}