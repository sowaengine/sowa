#include "animated_sprite_2d.hxx"

#include "core/app.hxx"
#include "core/time.hxx"

#include <iostream>

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include "resource/resource_manager.hxx"
#include "resource/sprite_sheet_animation/sprite_sheet_animation.hxx"

void AnimatedSprite2D::_start() {
	start();
}

// todo: sheet bounds checking
void AnimatedSprite2D::_update() {

	if (m_speed < 0.01f) {
		m_speed = 0.01f;
	}

	SpriteSheetAnimation *animation = ResourceManager::get().GetAs<SpriteSheetAnimation>(m_spritesheet);
	if (nullptr == animation)
		return;

	const Texture *currentTexture = nullptr;
	vec2 uv_xy1(0.f);
	vec2 uv_xy2(1.f);

	if (m_playing) {
		m_delta += Time::Delta();
		while (m_delta > m_speed) {
			m_delta -= m_speed;
			m_current_tile++;
		}
	}

	const Animation &currentAnim = animation->Animations().at(m_anim_index);
	m_current_tile %= currentAnim.tiles.size();

	const AnimationTile &tile = currentAnim.tiles[m_current_tile];
	const AnimationTexture &animTexture = animation->Textures().at(tile.texture_index);
	currentTexture = &animTexture.texture;

	float x_size = 1.f / animTexture.col;
	float y_size = 1.f / animTexture.row;

	uv_xy1 = vec2(x_size * tile.col_index, y_size * (animTexture.row - tile.row_index));
	uv_xy2 = uv_xy1 + vec2(x_size, y_size);

	if (nullptr == currentTexture)
		return;

	uint32_t textureID = currentTexture->ID();
	glm::mat4 model = calculate_transform();
	model = glm::scale(model, {currentTexture->Width() * (uv_xy2.x - uv_xy1.x), currentTexture->Height() * (uv_xy2.y - uv_xy1.y), 1.f});

	glm::vec4 points[4] = {
		{-0.5f, 0.5f, 0.f, 1.f},
		{-0.5f, -0.5f, 0.f, 1.f},
		{0.5f, -0.5f, 0.f, 1.f},
		{0.5f, 0.5f, 0.f, 1.f}};
	glm::vec2 uvs[4] = {
		{uv_xy1.x, uv_xy2.y},
		{uv_xy1.x, uv_xy1.y},
		{uv_xy2.x, uv_xy1.y},
		{uv_xy2.x, uv_xy2.y}};

	BatchVertex vertices[4];
	for (int i = 0; i < 4; i++) {
		points[i] = model * points[i];

		vertices[i].x = points[i].x;
		vertices[i].y = points[i].y;
		vertices[i].z = m_zindex;
		vertices[i].r = 1.f;
		vertices[i].g = 1.f;
		vertices[i].b = 1.f;
		vertices[i].a = 1.f;
		vertices[i].u = uvs[i].x;
		vertices[i].v = uvs[i].y;
		vertices[i].t_id = textureID;
		vertices[i].d_id = static_cast<float>(id());
		vertices[i].draw_mode = 0.f;
	}

	App::get().Renderer().PushQuad(vertices);
}

void AnimatedSprite2D::set_animation(const std::string &name, bool autostart) {
	set_animation(get_animation_index(name), autostart);
}
void AnimatedSprite2D::set_animation(uint32_t index, bool autostart) {
	m_anim_index = index;
	m_delta = 0.f;

	if (autostart)
		start();
}

uint32_t AnimatedSprite2D::get_animation_index(const std::string &name) {
	SpriteSheetAnimation *animation = ResourceManager::get().GetAs<SpriteSheetAnimation>(m_spritesheet);
	if (nullptr == animation)
		return -1;

	const std::vector<Animation> &anims = animation->Animations();
	for (size_t i = 0; i < anims.size(); i++) {
		if (anims[i].name == name) {
			return i;
		}
	}

	return -1;
}

void AnimatedSprite2D::start() {
	m_playing = true;
}

void AnimatedSprite2D::stop() {
	m_delta = 0.f;
	m_current_tile = 0;
	m_playing = false;
}

void AnimatedSprite2D::pause() {
	m_playing = false;
}