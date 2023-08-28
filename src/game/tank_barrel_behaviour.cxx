#include "game.hxx"

#include <iostream>

#include "core/rendering/gl.hxx"
#include "core/time.hxx"
#include "scene/node_2d.hxx"
#include "servers/input_server.hxx"
#include "servers/rendering_server.hxx"

static float lerpAngle(float from, float to, float t) {
	float diff = fmod(to - from, 3.141592653589 * 2);
	float dist = fmod(2.0 * diff, 3.141592653589 * 2) - diff;
	return from + dist * t;
}

static float speed = 20;

void TankBarrelBehaviour::Start(Node *node) {
}

void TankBarrelBehaviour::Update(Node *node) {
	Node2D *barrel = dynamic_cast<Node2D *>(node);
	if (nullptr == barrel) {
		return;
	}

	Node2D *tank = dynamic_cast<Node2D *>(node->GetParent());
	if (nullptr == tank) {
		return;
	}

	double x, y;
	InputServer::GetInstance().GetMousePosition(x, y);

	int w, h;
	RenderingServer::GetInstance().GetWindowSize(w, h);

	x = x * (1920.f / float(w));
	y = y * (1080.f / float(h));

	float targetRot = atan2(y - barrel->GlobalPosition().y, x - barrel->GlobalPosition().x);
	targetRot = targetRot - glm::radians(tank->GlobalRotation());

	barrel->Rotation() = glm::degrees(lerpAngle(glm::radians(barrel->Rotation()), targetRot, 0.2f));
}