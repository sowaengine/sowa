#include "game.hxx"

#include <iostream>

#include "core/app.hxx"
#include "core/behaviour/behaviour_db.hxx"
#include "core/time.hxx"
#include "resource/image_texture/image_texture.hxx"
#include "resource/resource_manager.hxx"
#include "scene/node_db.hxx"
#include "scene/scene.hxx"
#include "scene/sprite_2d.hxx"
#include "scene/text.hxx"
#include "servers/input_server.hxx"
#include "servers/rendering_server.hxx"

/* Temporary */

Scene *scene;

RID bulletTexture;
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

class MainScene : public Scene {
	void BeginScene() override {
		for (Node *node : Nodes()) {
			node->Start();
		}
	}

	void UpdateNode(Node *node) {
		if (App::GetInstance().IsRunning())
			node->UpdateBehaviours();
		node->Update();

		for (Node *child : node->GetChildren()) {
			child->Update();

			UpdateNode(child);
		}
	}

	void UpdateScene() override {
		for (Node *node : Nodes()) {
			UpdateNode(node);
		}
	}
};

void OnInput(InputEventMouseButton event);

void Main() {
	BehaviourDB::GetInstance().RegisterBehaviour("Tank Movement", Behaviour::New(TankMovement::Start, TankMovement::Update));
	BehaviourDB::GetInstance().RegisterBehaviour("Tank Barrel", Behaviour::New(TankBarrelBehaviour::Start, TankBarrelBehaviour::Update));
	BehaviourDB::GetInstance().RegisterBehaviour("Bullet Movement", Behaviour::New(BulletMovement::Start, BulletMovement::Update));

	scene = new MainScene;
	if (false) {
		Error err = scene->Load("res://scenes/game.escn");
		if (err != OK) {
			std::cout << "Failed to load scene" << std::endl;
		}

	} else
		load_scene();

	bulletTexture = ResourceManager::GetInstance().Load("res://assets/shotThin.png")->ResourceID();

	App::GetInstance().MouseInputCallback().append(OnInput);

	App::GetInstance().SetCurrentScene(scene);
}

void OnInput(InputEventMouseButton event) {
	if (!App::GetInstance().IsRunning())
		return;

	if (event.action == PRESSED && event.button == MB_LEFT) {
		Sprite2D *barrelSprite = dynamic_cast<Sprite2D *>(App::GetInstance().GetCurrentScene()->get_node_in_group("Barrel"));
		if (nullptr == barrelSprite) {
			return;
		}

		Node *bulletNode = NodeDB::GetInstance().Construct(NodeDB::GetInstance().GetNodeType("Sprite2D"));
		Sprite2D *bullet = dynamic_cast<Sprite2D *>(bulletNode);
		bullet->GetTexture() = bulletTexture;
		bullet->Position() = barrelSprite->GlobalPosition();
		bullet->Rotation() = barrelSprite->GlobalRotation();
		bullet->ZIndex() = 1.f;

		bullet->AddBehaviour("Bullet Movement");
		App::GetInstance().GetCurrentScene()->Nodes().push_back(bullet);
	}

	if (event.action == PRESSED && event.button == MB_RIGHT) {
		scene->Save("res://scenes/game.escn");
	}
}

void load_scene() {
	Node *centerNode = NodeDB::GetInstance().Construct(NodeDB::GetInstance().GetNodeType("Sprite2D"));
	Sprite2D *centerSprite = dynamic_cast<Sprite2D *>(centerNode);
	centerSprite->GetTexture() = 100;
	centerSprite->Position() = {600.f, 600.f};
	scene->Nodes().push_back(centerSprite);

	{
		Node *node = NodeDB::GetInstance().Construct(NodeDB::GetInstance().GetNodeType("Sprite2D"));
		Sprite2D *sprite = dynamic_cast<Sprite2D *>(node);
		sprite->GetTexture() = 100;
		sprite->Position() = {100.f, 100.f};
		sprite->Scale() = {0.75f, 0.75f};
		centerSprite->AddChild(sprite);
		centerSprite->AddBehaviour("Rotate Sprite");
	}

	Node *node = NodeDB::GetInstance().Construct(NodeDB::GetInstance().GetNodeType("Sprite2D"));
	Sprite2D *player = dynamic_cast<Sprite2D *>(node);
	player->GetTexture() = ResourceManager::GetInstance().Load("res://assets/tankBody_green_outline.png", 100)->ResourceID();
	player->Position() = {200.f, 200.f};
	player->Name() = "Player";
	player->AddBehaviour("Tank Movement");

	scene->Nodes()
		.push_back(player);

	node = NodeDB::GetInstance().Construct(NodeDB::GetInstance().GetNodeType("Sprite2D"));
	Sprite2D *barrel = dynamic_cast<Sprite2D *>(node);
	barrel->GetTexture() = ResourceManager::GetInstance().Load("res://assets/tankGreen_barrel2_outline.png", 101)->ResourceID();
	barrel->ZIndex() = 2.f;
	barrel->Name() = "Barrel";
	barrel->AddBehaviour("Tank Barrel");
	barrel->get_groups() = {"Barrel"};
	player->AddChild(barrel);

	bulletTexture = ResourceManager::GetInstance().Load("res://assets/shotThin.png", 102)->ResourceID();

	sandTexture = ResourceManager::GetInstance().Load("res://assets/tileGrass1.png", 103)->ResourceID();

	int index = 0;
	for (int x = 0;; x += ResourceManager::GetInstance().GetAs<Texture>(sandTexture)->Width()) {
		for (int y = 0;; y += ResourceManager::GetInstance().GetAs<Texture>(sandTexture)->Height()) {
			Node *node = NodeDB::GetInstance().Construct(NodeDB::GetInstance().GetNodeType("Sprite2D"));
			Sprite2D *sand = dynamic_cast<Sprite2D *>(node);
			sand->Position() = {x, y};
			sand->GetTexture() = sandTexture;
			sand->ZIndex() = -1.f;
			sand->Name() = "sand" + std::to_string(index++);

			scene->Nodes().push_back(sand);

			if (y > 1080.f)
				break;
		}
		if (x > 1920.f)
			break;
	}
}