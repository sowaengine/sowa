#include "game.hxx"

#include <iostream>

#include "core/app.hxx"
#include "core/input.hxx"
#include "core/rendering/gl.hxx"
#include "core/time.hxx"
#include "glm/glm.hpp"
#include "resource/resource_manager.hxx"
#include "scene/nodes.hxx"
#include "scene/scene_tree.hxx"
#include "servers/input_server.hxx"

static RID bulletTexture;

void MouseShooter::Start(Node *node, Behaviour *) {
	if (bulletTexture == 0)
		bulletTexture = ResourceManager::get().Load("res://assets/shotThin.png")->ResourceID();
}

void MouseShooter::Update(Node *node, Behaviour *) {

	if (Input::IsButtonJustPressed(MB_LEFT)) {
		Sprite2D *barrelSprite = dynamic_cast<Sprite2D *>(node);
		if (nullptr == barrelSprite) {
			return;
		}

		Node *bulletNode = SceneTree::get().get_scene()->create(NodeDB::get().get_node_type("Sprite2D"), "Bullet");
		Sprite2D *bullet = dynamic_cast<Sprite2D *>(bulletNode);
		bullet->texture() = bulletTexture;
		bullet->position() = barrelSprite->global_position();
		bullet->rotation() = barrelSprite->global_rotation();
		bullet->z_index() = 1.f;

		// bullet->add_behaviour("Bullet Movement");
		bullet->add_behaviour("Bullet");
		SceneTree::get().get_scene()->Root()->add_child(bullet);
	}
}