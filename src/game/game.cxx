#include "game.hxx"

#include <iostream>

#include "core/app.hxx"
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
Sprite2D *player = nullptr;
Sprite2D *barrel = nullptr;
Text *scoreText = nullptr;
float speed = 30.f;
float bulletSpeed = 200.f;

int score = 0;

std::vector<Sprite2D *> bullets;
RID bulletTexture;
RID sandTexture;

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

	void UpdateScene() override {

		for (Node *node : Nodes()) {
			node->Update();
		}

		return;

		glm::vec2 input(0.f, 0.f);

		if (InputServer::GetInstance().IsKeyDown(GLFW_KEY_W)) {
			input.y += 1.f;
		}
		if (InputServer::GetInstance().IsKeyDown(GLFW_KEY_S)) {
			input.y -= 1.f;
		}
		if (InputServer::GetInstance().IsKeyDown(GLFW_KEY_D)) {
			input.x += 1.f;
		}
		if (InputServer::GetInstance().IsKeyDown(GLFW_KEY_A)) {
			input.x -= 1.f;
		}

		if (glm::length(input) > 0.5f) {
			input = glm::normalize(input);
		}

		player->Position().x += input.x * speed * Time::Delta();
		player->Position().y += input.y * speed * Time::Delta();

		float targetRadians = glm::radians(player->Rotation());
		if (glm::length(input) > 0.5f) {
			targetRadians = atan2(input.y, input.x) + (3.141592653589 * 0.5);
		}
		player->Rotation() = glm::degrees(lerpAngle(glm::radians(player->Rotation()), targetRadians, 0.3f));

		barrel->Position() = player->Position();
		double x, y;
		InputServer::GetInstance().GetMousePosition(x, y);

		int w, h;
		RenderingServer::GetInstance().GetWindowSize(w, h);

		x = x * (1920.f / float(w));
		y = y * (1080.f / float(h));

		float targetRot = atan2(y - barrel->Position().y, x - barrel->Position().x) + (3.141592653589 * 0.5);

		barrel->Rotation() = glm::degrees(lerpAngle(glm::radians(barrel->Rotation()), targetRot, 0.2f));

		for (Sprite2D *bullet : bullets) {
			float angle = glm::radians(bullet->Rotation() + 90.f);
			float x = glm::cos(angle);
			float y = glm::sin(angle);

			bullet->Position().x += x * bulletSpeed * Time::Delta();
			bullet->Position().y += y * bulletSpeed * Time::Delta();
		}

		// scoreText->GetText() = "Score: " + std::to_string(score);
	}
};

void OnInput(InputEventMouseButton event);

void Main() {
	scene = new MainScene;
	// Error err = scene->Load("res://scenes/game.escn");
	// if (err != OK) {
	// 	std::cout << "Failed to load scene" << std::endl;
	// }
	// App::GetInstance().SetCurrentScene(scene);
	// return;

	Node *node = NodeDB::GetInstance().Construct(NodeDB::GetInstance().GetNodeType("Sprite2D"));
	player = dynamic_cast<Sprite2D *>(node);
	player->GetTexture() = ResourceManager::GetInstance().Load("res://assets/tankBody_green_outline.png", 100)->ResourceID();
	player->Position() = {200.f, 200.f};
	player->Name() = "Player";
	scene->Nodes().push_back(player);

	node = NodeDB::GetInstance().Construct(NodeDB::GetInstance().GetNodeType("Sprite2D"));
	barrel = dynamic_cast<Sprite2D *>(node);
	barrel->GetTexture() = ResourceManager::GetInstance().Load("res://assets/tankGreen_barrel2_outline.png", 101)->ResourceID();
	barrel->Position() = {200.f, 200.f};
	barrel->ZIndex() = 2.f;
	barrel->Name() = "Barrel";
	scene->Nodes().push_back(barrel);

	// scoreText = new Text;
	// scoreText->Position() = glm::vec2(10.f, 1000.f);
	// scene->Nodes().push_back(scoreText);

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

	App::GetInstance().MouseInputCallback().append(OnInput);

	App::GetInstance().SetCurrentScene(scene);
}

void OnInput(InputEventMouseButton event) {
	if (event.action == PRESSED && event.button == MB_LEFT) {
		Node *bulletNode = NodeDB::GetInstance().Construct(NodeDB::GetInstance().GetNodeType("Sprite2D"));
		Sprite2D *bullet = dynamic_cast<Sprite2D *>(bulletNode);
		bullet->GetTexture() = bulletTexture;
		bullets.push_back(bullet);
		bullet->Position() = barrel->Position();
		bullet->Rotation() = barrel->Rotation() + 180.f;
		bullet->ZIndex() = 1.f;

		float angle = glm::radians(bullet->Rotation() + 90.f);
		float x = glm::cos(angle);
		float y = glm::sin(angle);

		bullet->Position().x += x * bulletSpeed * Time::Delta();
		bullet->Position().y += y * bulletSpeed * Time::Delta();

		scene->Nodes().push_back(bullet);
	}
	if (event.action == PRESSED && event.button == MB_RIGHT) {

		Error err = scene->Save("res://scenes/game.escn");
		if (err != OK) {
			std::cout << err << std::endl;
		}

		bullets.clear();
		for (auto it = scene->Nodes().begin(); it != scene->Nodes().end();) {
			delete *it;
			scene->Nodes().erase(it);
		}
	}
}