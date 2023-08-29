#include "top_down_eight_dir.hxx"

#include "core/input.hxx"
#include "core/rendering/gl.hxx"
#include "core/time.hxx"
#include "glm/glm.hpp"
#include "scene/node.hxx"
#include "scene/node_2d.hxx"
#include "servers/input_server.hxx"

static float speed = 30.f;

static float lerpAngle(float from, float to, float t) {
	float diff = fmod(to - from, 3.141592653589 * 2);
	float dist = fmod(2.0 * diff, 3.141592653589 * 2) - diff;
	return from + dist * t;
}

void TopDownEightDirMovement::Start(Node *) {
}

void TopDownEightDirMovement::Update(Node *node) {
	Node2D *player = dynamic_cast<Node2D *>(node);
	if (nullptr == player) {
		return;
	}

	glm::vec2 input(0.f, 0.f);

	if (InputServer::GetInstance().IsKeyDown(KEY_W)) {
		input.y += 1.f;
	}
	if (InputServer::GetInstance().IsKeyDown(KEY_S)) {
		input.y -= 1.f;
	}
	if (InputServer::GetInstance().IsKeyDown(KEY_D)) {
		input.x += 1.f;
	}
	if (InputServer::GetInstance().IsKeyDown(KEY_A)) {
		input.x -= 1.f;
	}

	if (glm::length(input) > 0.5f) {
		input = glm::normalize(input);
	}

	player->Position().x += input.x * speed * Time::Delta();
	player->Position().y += input.y * speed * Time::Delta();

	float targetRadians = glm::radians(player->Rotation());
	if (glm::length(input) > 0.5f) {
		targetRadians = atan2(input.y, input.x);
	}
	player->Rotation() = glm::degrees(lerpAngle(glm::radians(player->Rotation()), targetRadians, 0.3f));
}