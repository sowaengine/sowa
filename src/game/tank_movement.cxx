#include "game.hxx"

#include <iostream>

#include "core/input.hxx"
#include "core/rendering/gl.hxx"
#include "core/time.hxx"
#include "glm/glm.hpp"
#include "scene/node_2d.hxx"
#include "servers/input_server.hxx"

static float acceleration = 500.f;
static float deceleration = 1000.f;

static float curAcceleration = 0.f;
static float maxAcceleration = 350.f;

static float lerpAngle(float from, float to, float t) {
	float diff = fmod(to - from, 3.141592653589 * 2);
	float dist = fmod(2.0 * diff, 3.141592653589 * 2) - diff;
	return from + dist * t;
}

void TankMovement::Start(Node *node, Behaviour *) {
	curAcceleration = 0.f;
}

void TankMovement::Update(Node *node, Behaviour *) {
	Node2D *player = dynamic_cast<Node2D *>(node);
	if (nullptr == player) {
		return;
	}

	glm::vec2 input(0.f, 0.f);

	if (Input::IsKeyDown(KEY_W)) {
		input.y += 1.f;
	}
	if (Input::IsKeyDown(KEY_S)) {
		input.y -= 1.f;
	}
	if (Input::IsKeyDown(KEY_D)) {
		input.x += 1.f;
	}
	if (Input::IsKeyDown(KEY_A)) {
		input.x -= 1.f;
	}

	if (glm::length(input) > 0.5f) {
		input = glm::normalize(input);

		curAcceleration = std::max(curAcceleration, 50.f);
		curAcceleration += acceleration * Time::Delta();
		curAcceleration = std::min(curAcceleration, maxAcceleration);
	} else {
		curAcceleration -= deceleration * Time::Delta();
		curAcceleration = std::max(curAcceleration, 0.f);
	}

	float targetRadians = glm::radians(player->rotation());
	if (glm::length(input) > 0.5f) {
		targetRadians = atan2(input.y, input.x);
	}
	player->rotation() = glm::degrees(lerpAngle(glm::radians(player->rotation()), targetRadians, 0.07f));

	float angle = glm::radians(player->rotation());
	float x = glm::cos(angle);
	float y = glm::sin(angle);

	player->position().x += x * curAcceleration * Time::Delta();
	player->position().y += y * curAcceleration * Time::Delta();
}