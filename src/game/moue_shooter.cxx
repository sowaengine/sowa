#include "game.hxx"

#include <iostream>

#include "core/app.hxx"
#include "core/input.hxx"
#include "core/rendering/gl.hxx"
#include "core/time.hxx"
#include "glm/glm.hpp"
#include "resource/resource_manager.hxx"
#include "scene/node_2d.hxx"
#include "scene/sprite_2d.hxx"
#include "servers/input_server.hxx"

static RID bulletTexture;

void MouseShooter::Start(Node *node, Behaviour *) {
	if (bulletTexture == 0)
		bulletTexture = ResourceManager::GetInstance().Load("res://assets/shotThin.png")->ResourceID();
}

void MouseShooter::Update(Node *node, Behaviour *) {

	if (Input::IsButtonJustPressed(MB_LEFT)) {
		Sprite2D *barrelSprite = dynamic_cast<Sprite2D *>(node);
		if (nullptr == barrelSprite) {
			return;
		}

		Node *bulletNode = App::GetInstance().GetCurrentScene()->New(NodeDB::GetInstance().GetNodeType("Sprite2D"));
		Sprite2D *bullet = dynamic_cast<Sprite2D *>(bulletNode);
		bullet->GetTexture() = bulletTexture;
		bullet->Position() = barrelSprite->GlobalPosition();
		bullet->Rotation() = barrelSprite->GlobalRotation();
		bullet->ZIndex() = 1.f;

		bullet->AddBehaviour("Bullet Movement");
		App::GetInstance().GetCurrentScene()->Nodes().push_back(bullet);
	}
}