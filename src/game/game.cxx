#include "game.hxx"

#include <iostream>

#include "core/app.hxx"
#include "core/behaviour/behaviour_db.hxx"
#include "core/time.hxx"
#include "resource/image_texture/image_texture.hxx"
#include "resource/resource_manager.hxx"
#include "scene/camera_2d.hxx"
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
	void StartNode(Node *node) {
		node->Start();
		node->StartBehaviours();

		for (Node *child : node->GetChildren()) {
			StartNode(child);
		}
	}

	void BeginScene() override {
		for (Node *node : Nodes()) {
			StartNode(node);
		}
	}

	void UpdateNode(Node *node) {
		if (App::GetInstance().IsRunning())
			node->UpdateBehaviours();
		node->Update();

		for (Node *child : node->GetChildren()) {

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
	if (true) {
		auto _ = Utils::ScopeTimer("Load Scene");

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

		Node *bulletNode = scene->New(NodeDB::GetInstance().GetNodeType("Sprite2D"));
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
	Node *centerNode = scene->New(NodeDB::GetInstance().GetNodeType("Sprite2D"));
	Sprite2D *centerSprite = dynamic_cast<Sprite2D *>(centerNode);
	centerSprite->GetTexture() = 100;
	centerSprite->Position() = {600.f, 600.f};
	scene->Nodes().push_back(centerSprite);

	{
		Node *node = scene->New(NodeDB::GetInstance().GetNodeType("Sprite2D"));
		Sprite2D *sprite = dynamic_cast<Sprite2D *>(node);
		sprite->GetTexture() = 100;
		sprite->Position() = {100.f, 100.f};
		sprite->Scale() = {0.75f, 0.75f};
		centerSprite->AddChild(sprite);
		centerSprite->AddBehaviour("Rotate Sprite");
	}

	Node *node = scene->New(NodeDB::GetInstance().GetNodeType("Sprite2D"));
	Sprite2D *player = dynamic_cast<Sprite2D *>(node);
	player->GetTexture() = scene->LoadResource("res://assets/tankBody_green_outline.png", 100)->ResourceID();
	player->Position() = {200.f, 200.f};
	player->Name() = "Player";
	player->AddBehaviour("Tank Movement");

	scene->Nodes()
		.push_back(player);

	node = scene->New(NodeDB::GetInstance().GetNodeType("Sprite2D"));
	Sprite2D *barrel = dynamic_cast<Sprite2D *>(node);
	barrel->GetTexture() = scene->LoadResource("res://assets/tankGreen_barrel2_outline.png", 101)->ResourceID();
	barrel->ZIndex() = 2.f;
	barrel->Name() = "Barrel";
	barrel->AddBehaviour("Tank Barrel");
	barrel->get_groups() = {"Barrel"};
	player->AddChild(barrel);

	Camera2D *cam = dynamic_cast<Camera2D *>(scene->New(NodeDB::GetInstance().GetNodeType("Camera2D"), "Camera"));
	player->AddChild(cam);
	scene->set_active_camera2d(cam->ID());

	bulletTexture = scene->LoadResource("res://assets/shotThin.png", 102)->ResourceID();

	sandTexture = scene->LoadResource("res://assets/tileGrass1.png", 103)->ResourceID();

	int index = 0;
	for (int x = 0;; x += ResourceManager::GetInstance().GetAs<Texture>(sandTexture)->Width()) {
		for (int y = 0;; y += ResourceManager::GetInstance().GetAs<Texture>(sandTexture)->Height()) {
			Node *node = scene->New(NodeDB::GetInstance().GetNodeType("Sprite2D"));
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