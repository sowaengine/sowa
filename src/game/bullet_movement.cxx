#include "core/time.hxx"
#include "game.hxx"
#include "scene/node_2d.hxx"

static float bulletSpeed = 2000.f;

void BulletMovement::Start(Node *, Behaviour *) {
}

void BulletMovement::Update(Node *node, Behaviour *) {
	Node2D *bullet = dynamic_cast<Node2D *>(node);
	if (nullptr == bullet) {
		return;
	}

	float angle = glm::radians(bullet->rotation());
	float x = glm::cos(angle);
	float y = glm::sin(angle);

	bullet->position().x += x * bulletSpeed * Time::Delta();
	bullet->position().y += y * bulletSpeed * Time::Delta();
}