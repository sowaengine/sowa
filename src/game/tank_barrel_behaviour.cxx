#include "game.hxx"

#include <iostream>

#include "core/app.hxx"
#include "core/rendering/gl.hxx"
#include "core/time.hxx"
#include "scene/camera_2d.hxx"
#include "scene/node_2d.hxx"
#include "servers/input_server.hxx"
#include "servers/rendering_server.hxx"

static float lerpAngle(float from, float to, float t) {
	float diff = fmod(to - from, 3.141592653589 * 2);
	float dist = fmod(2.0 * diff, 3.141592653589 * 2) - diff;
	return from + dist * t;
}

static float speed = 20;

void TankBarrelBehaviour::Start(Node *node, Behaviour *) {
}

void TankBarrelBehaviour::Update(Node *node, Behaviour *) {
	Node2D *barrel = dynamic_cast<Node2D *>(node);
	if (nullptr == barrel) {
		return;
	}

	Node2D *tank = dynamic_cast<Node2D *>(node->get_parent());
	if (nullptr == tank) {
		return;
	}

	double x, y;
	InputServer::get().GetMousePosition(x, y);

	int w, h;
	RenderingServer::get().GetWindowSize(w, h);

	x = x * (1920.f / float(w));
	y = y * (1080.f / float(h));

	vec2 midPoint = vec2(1920.f, 1080.f) * 0.5f;
	float targetRot = atan2(y - midPoint.y, x - midPoint.x);
	targetRot = targetRot - glm::radians(tank->global_rotation());

	barrel->rotation() = glm::degrees(lerpAngle(glm::radians(barrel->rotation()), targetRot, 0.2f));
}