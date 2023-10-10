#include "game.hxx"

#include <iostream>

#include "core/app.hxx"
#include "core/behaviour/behaviour_db.hxx"
#include "core/time.hxx"
#include "resource/image_texture/image_texture.hxx"
#include "resource/resource_manager.hxx"
#include "scene/node_db.hxx"
#include "scene/nodes.hxx"
#include "scene/scene.hxx"
#include "servers/input_server.hxx"
#include "servers/rendering_server.hxx"

/* Temporary */

Scene *scene;

RID sandTexture;

static void load_scene();

float lerp(float from, float to, float t) {
	return from + ((to - from) * t);
}

float lerpAngle(float from, float to, float t) {
	float diff = fmod(to - from, 3.141592653589 * 2);
	float dist = fmod(2.0 * diff, 3.141592653589 * 2) - diff;
	return from + dist * t;
}

void OnInput(InputEventMouseButton event);

void Main() {
	BehaviourDB::get().RegisterBehaviour("Tank Movement", Behaviour::New(TankMovement::Start, TankMovement::Update));
	BehaviourDB::get().RegisterBehaviour("Mouse Shooter", Behaviour::New(MouseShooter::Start, MouseShooter::Update));

	scene = new Scene;
	if (true) {
		auto _ = Utils::ScopeTimer("Load Scene");

		ErrorCode err = scene->load("res://scenes/game.escn");
		if (err != OK) {
			std::cout << "Failed to load scene" << std::endl;
		}

		/*
		Node *crates = scene->create(NodeDB::get().get_node_type("Node"), "Crates", 300);
		for (int i = 0; i < 10; i++) {
			Sprite2D *sprite = dynamic_cast<Sprite2D *>(scene->create(NodeDB::get().get_node_type("Sprite2D"), Utils::Format("wood {}", i), 301 + i));
			sprite->position().x = 300;
			sprite->position().y = i * 120;
			sprite->rotation() = Utils::RandRangeFloat(-20.f, 20.f);
			sprite->scale() = 1.5f;
			sprite->texture() = 51;

			crates->add_child(sprite);
		}

		for (int i = 0; i < 10; i++) {
			Sprite2D *sprite = dynamic_cast<Sprite2D *>(scene->create(NodeDB::get().get_node_type("Sprite2D"), Utils::Format("metal {}", i), 311 + i));
			sprite->position().x = 1200;
			sprite->position().y = i * 120;
			sprite->rotation() = Utils::RandRangeFloat(-20.f, 20.f);
			sprite->scale() = 1.5f;
			sprite->texture() = 52;

			crates->add_child(sprite);
		}
		scene->Nodes().push_back(crates);
		*/
	} else
		load_scene();

	App::get().MouseInputCallback().append(OnInput);

	App::get().SetCurrentScene(scene);
}

void OnInput(InputEventMouseButton event) {
	if (!App::get().IsRunning())
		return;

	if (event.action == PRESSED && event.button == MB_RIGHT) {
		// scene->Save("res://scenes/game.escn");
	}
}

void load_scene() {
	Node *centerNode = scene->create(NodeDB::get().get_node_type("Sprite2D"));
	Sprite2D *centerSprite = dynamic_cast<Sprite2D *>(centerNode);
	centerSprite->texture() = 100;
	centerSprite->position() = {600.f, 600.f};
	scene->Nodes().push_back(centerSprite);

	{
		Node *node = scene->create(NodeDB::get().get_node_type("Sprite2D"));
		Sprite2D *sprite = dynamic_cast<Sprite2D *>(node);
		sprite->texture() = 100;
		sprite->position() = {100.f, 100.f};
		sprite->scale() = {0.75f, 0.75f};
		centerSprite->add_child(sprite);
		centerSprite->add_behaviour("Rotate Sprite");
	}

	Node *node = scene->create(NodeDB::get().get_node_type("Sprite2D"));
	Sprite2D *player = dynamic_cast<Sprite2D *>(node);
	player->texture() = scene->load_resource("res://assets/tankBody_green_outline.png", 100)->ResourceID();
	player->position() = {200.f, 200.f};
	player->name() = "Player";
	player->add_behaviour("Tank Movement");

	scene->Nodes()
		.push_back(player);

	node = scene->create(NodeDB::get().get_node_type("Sprite2D"));
	Sprite2D *barrel = dynamic_cast<Sprite2D *>(node);
	barrel->texture() = scene->load_resource("res://assets/tankGreen_barrel2_outline.png", 101)->ResourceID();
	barrel->z_index() = 2.f;
	barrel->name() = "Barrel";
	barrel->add_behaviour("Tank Barrel");
	barrel->add_behaviour("Mouse Shooter");
	barrel->get_groups() = {"Barrel"};
	player->add_child(barrel);

	Camera2D *cam = dynamic_cast<Camera2D *>(scene->create(NodeDB::get().get_node_type("Camera2D"), "Camera"));
	player->add_child(cam);
	scene->set_active_camera2d(cam->id());

	sandTexture = scene->load_resource("res://assets/tileGrass1.png", 103)->ResourceID();

	int index = 0;
	for (int x = 0;; x += ResourceManager::get().GetAs<Texture>(sandTexture)->Width()) {
		for (int y = 0;; y += ResourceManager::get().GetAs<Texture>(sandTexture)->Height()) {
			Node *node = scene->create(NodeDB::get().get_node_type("Sprite2D"));
			Sprite2D *sand = dynamic_cast<Sprite2D *>(node);
			sand->position() = {x, y};
			sand->texture() = sandTexture;
			sand->z_index() = -1.f;
			sand->name() = "sand" + std::to_string(index++);

			scene->Nodes().push_back(sand);

			if (y > 1080.f)
				break;
		}
		if (x > 1920.f)
			break;
	}
}