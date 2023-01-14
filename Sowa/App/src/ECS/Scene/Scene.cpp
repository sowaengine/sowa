#include "ECS/Scene/Scene.hpp"
#include "Core/Application.hpp"
#include "Core/ProjectSettings.hpp"
#include "Debug.hpp"
#include "ECS/Components/Components.hpp"
#include "Resource/ResourceLoader.hpp"
#include "Resource/ResourceManager.hpp"
#include "Utils/File.hpp"
#include "Utils/YAML.hpp"
// #include "box2d/box2d.h"
#include "yaml-cpp/yaml.h"

#include "stlpch.hpp"

#define CALL_FOR_ALL_COMPONENTS(func, ...)                    \
	do {                                                      \
		func<Sowa::Component::AnimatedSprite2D>(__VA_ARGS__); \
		func<Sowa::Component::Button>(__VA_ARGS__);           \
		func<Sowa::Component::Camera2D>(__VA_ARGS__);         \
		func<Sowa::Component::Group>(__VA_ARGS__);            \
		func<Sowa::Component::NinePatchRect>(__VA_ARGS__);    \
		func<Sowa::Component::PhysicsBody2D>(__VA_ARGS__);    \
		func<Sowa::Component::Sprite2D>(__VA_ARGS__);         \
		func<Sowa::Component::Text2D>(__VA_ARGS__);           \
		func<Sowa::Component::Transform2D>(__VA_ARGS__);      \
		func<Sowa::Component::UITransform>(__VA_ARGS__);      \
		func<Sowa::Component::ScriptContainer>(__VA_ARGS__);  \
	} while (0)

namespace Sowa {
void Scene::StartScene() {
	m_SceneOldCamera2D.camera2d = m_SceneCamera2D.camera2d;
	m_SceneOldCamera2D.transform2d = m_SceneCamera2D.transform2d;
	m_SceneCamera2D.transform2d = Sowa::Component::Transform2D();
	m_SceneCamera2D.camera2d = Sowa::Component::Camera2D();

	// m_pPhysicsWorld2D = std::make_shared<b2World>(b2Vec2(m_Gravity.x, m_Gravity.y));

	{
		auto view = m_Registry.view<Component::Transform2D, Component::PhysicsBody2D>();
		for (auto &e : view) {
			Entity entity(e, &m_Registry);
			auto &tc = entity.GetComponent<Component::Transform2D>();
			auto &body = entity.GetComponent<Component::PhysicsBody2D>();

			// body.m_b2BodyDef.type = body.GetInternalBodyType();
			// body.m_b2BodyDef.position.Set(SCREEN_TO_WORLD(tc.Position().x), SCREEN_TO_WORLD(tc.Position().y));
			// body.m_b2BodyDef.angle = -tc.Rotation() * DEG2RAD;
			//
			// body.m_b2Body = m_pPhysicsWorld2D->CreateBody(&body.m_b2BodyDef);

			// for (Collider2D &collider : body.Colliders()) {
			// if (collider.shape == ColliderShape2D::BOX) {
			// 	collider.polyShape.SetAsBox(
			// 		SCREEN_TO_WORLD(static_cast<float>(collider.width) / 2.f), SCREEN_TO_WORLD(static_cast<float>(collider.height) / 2.f),
			// 		b2Vec2(SCREEN_TO_WORLD(collider.offset.x), SCREEN_TO_WORLD(collider.offset.y)),
			// 		DEG2RAD * collider.rotation);

			// 	collider.fixtureDef.shape = &collider.polyShape;
			// }
			// if (collider.shape == ColliderShape2D::CIRCLE) {
			// 	collider.circleShape.m_p = {SCREEN_TO_WORLD(collider.offset.x), SCREEN_TO_WORLD(collider.offset.y)};
			// 	collider.circleShape.m_radius = SCREEN_TO_WORLD(static_cast<float>(collider.radius));

			// 	collider.fixtureDef.shape = &collider.circleShape;
			// }
			// collider.fixtureDef.density = collider.density;
			// collider.fixtureDef.friction = collider.friction;
			// collider.fixtureDef.restitution = collider.restitution;
			// collider.fixtureDef.restitutionThreshold = collider.restitutionThreshold;
			// collider.fixture = body.m_b2Body->CreateFixture(&collider.fixtureDef);
			// }
		}
	}
}

void Scene::StopScene() {
	m_SceneCamera2D.camera2d = m_SceneOldCamera2D.camera2d;
	m_SceneCamera2D.transform2d = m_SceneOldCamera2D.transform2d;

	// m_pPhysicsWorld2D = nullptr;
}

template <typename T>
void CopyComponent(Sowa::Entity &srcEntity, Sowa::Entity &dstEntity) {
	if (srcEntity.HasComponent<T>()) {
		auto &srcComponent = srcEntity.GetComponent<T>();
		auto &dstComponent = dstEntity.AddComponent<T>();

		dstComponent = srcComponent;
	}
}

// static
void Scene::CopyScene(Scene &src, Scene &dst) {
	dst.m_Registry.clear();
	auto view = src.m_Registry.view<Sowa::Component::Common>();
	for (const auto &entityID : view) {
		Entity srcEntity(entityID, &src.m_Registry);
		Entity dstEntity = dst.Create(srcEntity.GetComponent<Sowa::Component::Common>().Name(), (uint32_t)entityID);

		CALL_FOR_ALL_COMPONENTS(CopyComponent, srcEntity, dstEntity);
	}
}

void Scene::CopyEntity(Sowa::Entity entity) {
	ClearCopiedEntities();
	AddCopiedEntity(entity);
}

void Scene::AddCopiedEntity(Sowa::Entity entity) {
	entt::entity copyID = m_CopyRegistry.create((entt::entity)entity.ID());
	Sowa::Entity copyEntity(copyID, &m_CopyRegistry);

	// Copy components
	auto &common = copyEntity.AddComponent<Component::Common>();
	common.Name() = entity.GetComponent<Component::Common>().Name();

	CopyComponent<Component::AnimatedSprite2D>(entity, copyEntity);
	CopyComponent<Component::Group>(entity, copyEntity);
	CopyComponent<Component::Sprite2D>(entity, copyEntity);
	CopyComponent<Component::Text2D>(entity, copyEntity);
	CopyComponent<Component::Transform2D>(entity, copyEntity);
}

void Scene::ClearCopiedEntities() {
	m_CopyRegistry.clear();
}

/**
 * @brief Create an Entity From {srcRegistry} on {scene}
 */
Sowa::Entity CreateEntityFromRegistry(entt::entity id, entt::registry &srcRegistry, Sowa::Scene &scene) {
	Sowa::Entity copyEntity(id, &srcRegistry);
	Sowa::Entity entity = scene.Create(copyEntity.GetComponent<Component::Common>().Name());

	// Copy components
	CopyComponent<Component::AnimatedSprite2D>(copyEntity, entity);
	CopyComponent<Component::Group>(copyEntity, entity);
	CopyComponent<Component::Sprite2D>(copyEntity, entity);
	CopyComponent<Component::Text2D>(copyEntity, entity);
	CopyComponent<Component::Transform2D>(copyEntity, entity);

	return entity;
}

Sowa::Entity Scene::PasteCopiedEntity() {
	if (m_CopyRegistry.size() < 1)
		throw std::runtime_error("There is no copied entity");

	entt::entity copyEntityID = m_CopyRegistry.view<Component::Common>()[0];
	Sowa::Entity newEntity = CreateEntityFromRegistry(copyEntityID, m_CopyRegistry, *this);
	return newEntity;
}

std::vector<Sowa::Entity> Scene::PasteCopiedEntities() {
	if (m_CopyRegistry.size() < 1)
		throw std::runtime_error("There are no copied entities");

	std::vector<Sowa::Entity> entities;
	auto view = m_CopyRegistry.view<Component::Common>();
	for (int i = 0; view.size(); i++) {
		entities.push_back(CreateEntityFromRegistry(view[i], m_CopyRegistry, *this));
	}
	return entities;
}

uint32_t Scene::GetCopiedEntityCount() {
	return m_CopyRegistry.size();
}

Scene::Scene() {
}

Scene::~Scene() {
}

Entity Scene::Create(const std::string &name, EntityID _id /* = 0*/) {
	Entity entity;
	EntityID id = _id;

	// if id == 0 or there is an entity with given id, create entity with generated id
	if (id == 0 || m_Registry.valid((entt::entity)id))
		return Create(name, Random::GenerateID31());

	entity.SetEntityID(m_Registry.create((entt::entity)id));
	entity.SetRegistry(&m_Registry);

	auto &common = entity.AddComponent<Component::Common>();
	common.Name() = name;

	return entity;
}

void Scene::Destroy(Sowa::Entity &entity) {
	if (m_Registry.valid((entt::entity)entity.ID())) {
		m_Registry.destroy((entt::entity)entity.ID());
	}
	entity.SetEntityID(entt::null);
	entity.SetRegistry(nullptr);
}

static void YAMLSerializeEntity(Sowa::Entity entity, YAML::Emitter &yaml) {
	yaml << YAML::Key << entity.ID() << YAML::Value << YAML::BeginMap;
	yaml << YAML::Key << "Name" << YAML::Value << entity.GetComponent<Sowa::Component::Common>().Name();

	// <Components>
	yaml << YAML::Key << "Components" << YAML::BeginMap;
	// <AnimatedSprite2D>
	if (entity.HasComponent<Component::AnimatedSprite2D>()) {
		Component::AnimatedSprite2D &component = entity.GetComponent<Component::AnimatedSprite2D>();

		yaml << YAML::Key << "AnimatedSprite2D" << YAML::BeginMap;
		yaml << YAML::Key << "Animations" << YAML::Value << component.Animations();
		yaml << YAML::EndMap;
	}
	// </AnimatedSprite2D>
	// <Button>
	if (entity.HasComponent<Component::Button>()) {
		Component::Button &component = entity.GetComponent<Component::Button>();

		yaml << YAML::Key << "Button" << YAML::BeginMap;
		yaml << YAML::Key << "Text" << YAML::Value << component.Text();
		yaml << YAML::Key << "Visible" << YAML::Value << component.Visible();
		yaml << YAML::Key << "Disabled" << YAML::Value << component.Disabled();
		yaml << YAML::EndMap;
	}
	// </Button>
	// <Camera2D>
	if (entity.HasComponent<Component::Camera2D>()) {
		Component::Camera2D &component = entity.GetComponent<Component::Camera2D>();

		yaml << YAML::Key << "Camera2D" << YAML::BeginMap;
		yaml << YAML::Key << "Current" << YAML::Value << component.Current();
		yaml << YAML::Key << "Rotatable" << YAML::Value << component.Rotatable();
		yaml << YAML::Key << "Zoom" << YAML::Value << component.Zoom();
		yaml << YAML::Key << "Center" << YAML::Value << component.Center();
		yaml << YAML::EndMap;
	}
	// </Camera2D>
	// <Group>
	if (entity.HasComponent<Component::Group>()) {
		Component::Group &component = entity.GetComponent<Component::Group>();

		yaml << YAML::Key << "Group" << YAML::BeginMap;
		yaml << YAML::Key << "Groups" << YAML::Value << component.Groups();
		yaml << YAML::EndMap;
	}
	// </Group>
	// <PhysicsBody2D>
	if (entity.HasComponent<Component::PhysicsBody2D>()) {
		Component::PhysicsBody2D &component = entity.GetComponent<Component::PhysicsBody2D>();

		yaml << YAML::Key << "PhysicsBody2D" << YAML::BeginMap;
		yaml << YAML::Key << "BodyType" << YAML::Value << component.BodyType();
		yaml << YAML::Key << "Colliders" << YAML::Value << YAML::BeginSeq;
		for (Collider2D &collider : component.Colliders()) {
			yaml << YAML::BeginMap;
			yaml << YAML::Key << "Shape" << YAML::Value << collider.shape;
			// yaml << YAML::Key << "Offset" << YAML::Value << collider.offset;
			yaml << YAML::Key << "Rotation" << YAML::Value << collider.rotation;

			if (collider.shape == ColliderShape2D::BOX) {
				yaml << YAML::Key << "Width" << YAML::Value << collider.width;
				yaml << YAML::Key << "Height" << YAML::Value << collider.height;
			} else if (collider.shape == ColliderShape2D::CIRCLE) {
				yaml << YAML::Key << "Radius" << YAML::Value << collider.radius;
			}

			yaml << YAML::Key << "Density" << YAML::Value << collider.density;
			yaml << YAML::Key << "Friction" << YAML::Value << collider.friction;
			yaml << YAML::Key << "Restitution" << YAML::Value << collider.restitution;
			yaml << YAML::Key << "RestitutionThreshold" << YAML::Value << collider.restitutionThreshold;
			yaml << YAML::EndMap;
		}
		yaml << YAML::EndSeq;

		yaml << YAML::EndMap;
	}
	// </PhysicsBody2D>
	// <ScriptContainer>
	if (entity.HasComponent<Component::ScriptContainer>()) {
		Component::ScriptContainer &component = entity.GetComponent<Component::ScriptContainer>();

		yaml << YAML::Key << "ScriptContainer" << YAML::BeginMap;
		yaml << YAML::Key << "LuaScripts" << YAML::Value << component._LuaScripts;
		yaml << YAML::EndMap;
	}
	// </ScriptContainer>
	// <Sprite2D>
	if (entity.HasComponent<Component::Sprite2D>()) {
		Component::Sprite2D &component = entity.GetComponent<Component::Sprite2D>();

		yaml << YAML::Key << "Sprite2D" << YAML::BeginMap;
		if (component.Texture() != nullptr)
			yaml << YAML::Key << "Texture" << YAML::Value << component.Texture()->GetResourceID();
		yaml << YAML::Key << "Visible" << YAML::Value << component.Visible();
		yaml << YAML::EndMap;
	}
	// </Sprite2D>
	// <Text2D>
	if (entity.HasComponent<Component::Text2D>()) {
		Component::Text2D &component = entity.GetComponent<Component::Text2D>();

		yaml << YAML::Key << "Text2D" << YAML::BeginMap;
		yaml << YAML::Key << "Text" << YAML::Value << component.Text();
		yaml << YAML::Key << "Color" << YAML::Value << component.Color();
		yaml << YAML::Key << "FontSize" << YAML::Value << component.FontSize();
		yaml << YAML::Key << "Visible" << YAML::Value << component.Visible();
		yaml << YAML::EndMap;
	}
	// </Text2D>
	// <Transform2D>
	if (entity.HasComponent<Component::Transform2D>()) {
		Component::Transform2D &component = entity.GetComponent<Component::Transform2D>();

		yaml << YAML::Key << "Transform2D" << YAML::BeginMap;
		yaml << YAML::Key << "Position" << YAML::Value << component.Position();
		yaml << YAML::Key << "Scale" << YAML::Value << component.Scale();
		yaml << YAML::Key << "Rotation" << YAML::Value << component.Rotation();
		// yaml << YAML::Key << "ZIndex" << YAML::Value << component.ZIndex();
		yaml << YAML::EndMap;
	}
	// </Transform2D>
	// <UITransform>
	if (entity.HasComponent<Component::UITransform>()) {
		Component::UITransform &component = entity.GetComponent<Component::UITransform>();

		yaml << YAML::Key << "UITransform" << YAML::BeginMap;
		yaml << YAML::Key << "Position" << YAML::Value << component.Position();
		yaml << YAML::Key << "Scale" << YAML::Value << component.Scale();
		yaml << YAML::Key << "Size" << YAML::Value << component.Size();
		yaml << YAML::Key << "Rotation" << YAML::Value << component.Rotation();
		// yaml << YAML::Key << "ZIndex" << YAML::Value << component.ZIndex();
		yaml << YAML::EndMap;
	}
	// </UITransform>

	yaml << YAML::EndMap;
	// </Components>

	yaml << YAML::Newline;
	yaml << YAML::EndMap;
}

bool Scene::Save() {
	return SaveToFile(path.string().c_str());
}
bool Scene::SaveToFile(const char *file) {
	std::filesystem::path savepath = Sowa::File::Path(file);
	YAML::Emitter yaml;
	yaml << YAML::BeginMap;
	yaml << YAML::Key << "Type" << YAML::Value << "Scene";
	yaml << YAML::Newline;
	yaml << YAML::Newline;
	// <Resources>
	yaml << YAML::Key << "ResourceList" << YAML::Value;
	yaml << YAML::BeginMap;
	{ // <Sowa::Texture>
		yaml << YAML::Key << "Texture" << YAML::Value << YAML::BeginMap;
		ResourceManager<Sowa::ImageTexture> &loader_texture = GetResourceManager<Sowa::ImageTexture>();
		std::map<ResourceID, Reference<Sowa::ImageTexture>> &textures = loader_texture.GetResources();

		// remove unused resources
		for (auto it = textures.cbegin(), next_it = it; it != textures.cend(); it = next_it) {
			++next_it;
			if ((*it).second.use_count() == 1) {
				Reference<ImageTexture> res = (*it).second;
				Debug::Log("Removed unused resource from scene[type:'{}', id:'{}', path:'{}']", "Texture", res->GetResourceID(), res->GetFilepath());
				textures.erase(it);
			}
		}

		for (auto [id, texture] : textures) {
			if (texture == nullptr) {
				Debug::Error("Unknown texture with id {}", id);
				continue;
			}

			yaml << YAML::Key << id << YAML::Value << YAML::BeginMap;
			yaml << YAML::Key << "Path" << YAML::Value << texture->GetFilepath();
			yaml << YAML::Newline;
			yaml << YAML::EndMap;
		}

		yaml << YAML::EndMap;
	} // </Sowa::Texture>
	{ // <Sowa::SpriteSheetAnimation>
		yaml << YAML::Key << "SpriteSheetAnimation" << YAML::Value << YAML::BeginMap;
		ResourceManager<Sowa::SpriteSheetAnimation> &loader_anims = GetResourceManager<Sowa::SpriteSheetAnimation>();
		std::map<ResourceID, std::shared_ptr<Sowa::SpriteSheetAnimation>> &anims = loader_anims.GetResources();

		for (auto [id, anim] : anims) {
			yaml << YAML::Key << id << YAML::Value << YAML::BeginMap;
			yaml << YAML::Key << "HFrames" << YAML::Value << anim->HFrames();
			yaml << YAML::Key << "VFrames" << YAML::Value << anim->VFrames();
			yaml << YAML::Key << "SelectedRow" << YAML::Value << anim->SelectedRow();
			yaml << YAML::Key << "FrameCount" << YAML::Value << anim->FrameCount();
			yaml << YAML::Key << "StartFrame" << YAML::Value << anim->StartFrame();
			yaml << YAML::Key << "FPS" << YAML::Value << anim->FPS();

			yaml << YAML::Key << "Texture" << YAML::Value << anim->Texture();
			yaml << YAML::Newline;
			yaml << YAML::EndMap;
		}

		yaml << YAML::EndMap;
	} // </Sowa::SpriteSheetAnimation>
	yaml << YAML::Newline << YAML::EndMap;
	// </Resources>

	yaml << YAML::Key << "EntityList" << YAML::Value;
	yaml << YAML::BeginMap;
	auto view = m_Registry.view<Sowa::Component::Common>();
	for (auto it = view.rbegin(); it < view.rend(); ++it)
		YAMLSerializeEntity(Sowa::Entity(*it, &m_Registry), yaml);
	yaml << YAML::EndMap;
	yaml << YAML::EndMap;

	std::ofstream ofstream(savepath);
	ofstream << yaml.c_str();

	Debug::Log("Saved scene to {}", savepath.string());
	return true;
}

bool Scene::LoadFromFile(const char *file) {
	m_Registry.clear();

	ClearResourceManager<Sowa::ImageTexture>();
	ClearResourceManager<Sowa::SpriteSheetAnimation>();

	std::filesystem::path inpath = Sowa::File::Path(file);
	path = file;
	Debug::Log("Loading scene from {}", inpath.string());

	YAML::Node node = YAML::LoadFile(inpath.string());
	if (node["Type"].as<std::string>("") != "Scene") {
		Debug::Error("Wrong file type: '{}', expected 'Scene'", node["Type"].as<std::string>(""));
		return false;
	}
	YAML::Node resources = node["ResourceList"];
	if (resources) {
		{ // <Sowa::Texture>
			if (YAML::Node resource_node = resources["Texture"]; resource_node) {
				ResourceManager<Sowa::ImageTexture> &resource_loader = GetResourceManager<Sowa::ImageTexture>();
				for (auto it = resource_node.begin(); it != resource_node.end(); ++it) {
					ResourceID id = it->first.as<ResourceID>(0);
					YAML::Node tex_node = it->second;
					std::string tex_path = tex_node["Path"].as<std::string>("");
					if (tex_path.size() > 0) {
						Reference<ImageTexture> tex = resource_loader.LoadResource(tex_path.c_str(), id);
					}
				}
			}
		} // </Sowa::Texture>
		{ // <Sowa::SpriteSheetAnimation>
			if (YAML::Node resource_node = resources["SpriteSheetAnimation"]; resource_node) {
				ResourceManager<Sowa::SpriteSheetAnimation> &resource_loader = GetResourceManager<Sowa::SpriteSheetAnimation>();
				for (auto it = resource_node.begin(); it != resource_node.end(); ++it) {
					ResourceID id = it->first.as<ResourceID>(0);
					YAML::Node tex_node = it->second;

					Reference<Sowa::SpriteSheetAnimation> anim = resource_loader.LoadResource("", id);
					anim->HFrames() = tex_node["HFrames"].as<int>(0);
					anim->VFrames() = tex_node["VFrames"].as<int>(0);
					anim->SelectedRow() = tex_node["SelectedRow"].as<int>(0);
					anim->FrameCount() = tex_node["FrameCount"].as<int>(0);
					anim->StartFrame() = tex_node["StartFrame"].as<int>(0);
					anim->FPS() = tex_node["FPS"].as<int>(0);

					anim->Texture() = tex_node["Texture"].as<ResourceID>(0);
				}
			}
		} // </Sowa::SpriteSheetAnimation>
	}

	YAML::Node entities = node["EntityList"];
	if (entities) {
		for (auto it = entities.begin(); it != entities.end(); ++it) {
			uint32_t id = it->first.as<uint32_t>(0);
			YAML::Node entity_node = it->second;
			std::string name = entity_node["Name"].as<std::string>("Entity");

			Sowa::Entity entity = Create(name, id);
			YAML::Node components = entity_node["Components"];
			if (components) {
				if (YAML::Node component_node = components["AnimatedSprite2D"]; component_node) {
					Component::AnimatedSprite2D &component = entity.AddComponent<Component::AnimatedSprite2D>();
					component.Animations() = component_node["Animations"].as<std::map<std::string, ResourceID>>();

					if (std::string anim_name = component_node["SelectedAnimation"].as<std::string>(""); anim_name != "")
						component.SelectAnimation(anim_name);
				}
				if (YAML::Node component_node = components["Button"]; component_node) {
					Component::Button &component = entity.AddComponent<Component::Button>();
					component.Text() = component_node["Text"].as<std::string>(std::string{""});
					component.Visible() = component_node["Visible"].as<bool>(true);
					component.Disabled() = component_node["Disabled"].as<bool>(false);
				}
				if (YAML::Node component_node = components["Camera2D"]; component_node) {
					Component::Camera2D &component = entity.AddComponent<Component::Camera2D>();
					component.Current() = component_node["Current"].as<bool>(false);
					component.Rotatable() = component_node["Rotatable"].as<bool>(true);
					component.Zoom() = component_node["Zoom"].as<float>(1.f);
					component.Center() = component_node["Center"].as<Vec2>(Vec2{0.5f, 0.5f});
				}
				if (YAML::Node component_node = components["Group"]; component_node) {
					Component::Group &component = entity.AddComponent<Component::Group>();
					component.Groups() = component_node["Groups"].as<std::vector<std::string>>(std::vector<std::string>{});
				}
				if (YAML::Node component_node = components["PhysicsBody2D"]; component_node) {
					Component::PhysicsBody2D &component = entity.AddComponent<Component::PhysicsBody2D>();
					component.BodyType() = component_node["BodyType"].as<PhysicsBodyType>(component.BodyType());
					std::vector<Collider2D> &colliders = component.Colliders();
					if (component_node["Colliders"]) {
						for (size_t i = 0; i < component_node["Colliders"].size(); i++) {
							YAML::Node yaml_collider = component_node["Colliders"][i];
							Collider2D collider;
							collider.shape = yaml_collider["Shape"].as<ColliderShape2D>(collider.shape);
							// collider.offset = yaml_collider["Offset"].as<b2Vec2>(collider.offset);
							collider.rotation = yaml_collider["Rotation"].as<float>(collider.rotation);
							collider.width = yaml_collider["Width"].as<float>(static_cast<float>(collider.width));
							collider.height = yaml_collider["Height"].as<float>(static_cast<float>(collider.height));
							collider.radius = yaml_collider["Radius"].as<float>(static_cast<float>(collider.radius));
							collider.density = yaml_collider["Density"].as<float>(collider.density);
							collider.friction = yaml_collider["Friction"].as<float>(collider.friction);
							collider.restitution = yaml_collider["Restitution"].as<float>(collider.restitution);
							collider.restitutionThreshold = yaml_collider["RestitutionThreshold"].as<float>(collider.restitutionThreshold);
							colliders.push_back(collider);
						}
					}
				}
				if (YAML::Node component_node = components["ScriptContainer"]; component_node) {
					Component::ScriptContainer &component = entity.AddComponent<Component::ScriptContainer>();
					component._LuaScripts = component_node["LuaScripts"].as<std::vector<std::string>>(std::vector<std::string>{});
				}
				if (YAML::Node component_node = components["Sprite2D"]; component_node) {
					Component::Sprite2D &component = entity.AddComponent<Component::Sprite2D>();
					ResourceID id = component_node["Texture"].as<ResourceID>(0);
					if (id != 0)
						component.Texture() = GetResourceManager<Sowa::ImageTexture>().GetResource(id);
					component.Visible() = component_node["Visible"].as<bool>(true);
				}
				if (YAML::Node component_node = components["Text2D"]; component_node) {
					Component::Text2D &component = entity.AddComponent<Component::Text2D>();
					component.Text() = component_node["Text"].as<std::string>("");
					component.Color() = component_node["Color"].as<glm::vec4>(glm::vec4{255.f, 255.f, 255.f, 255.f});
					component.FontSize() = component_node["FontSize"].as<float>(64.f);
					component.Visible() = component_node["Visible"].as<bool>(true);
					// component.ZIndex() = component_node["ZIndex"].as<int>(0);
				}
				if (YAML::Node component_node = components["Transform2D"]; component_node) {
					Component::Transform2D &component = entity.AddComponent<Component::Transform2D>();
					component.Position() = component_node["Position"].as<glm::vec2>(glm::vec2{0.f, 0.f});
					component.Scale() = component_node["Scale"].as<glm::vec2>(glm::vec2{1.f, 1.f});
					component.Rotation() = component_node["Rotation"].as<float>(0.f);
					// component.ZIndex() = component_node["ZIndex"].as<int>(0);
				}
				if (YAML::Node component_node = components["UITransform"]; component_node) {
					Component::UITransform &component = entity.AddComponent<Component::UITransform>();
					component.Position() = component_node["Position"].as<glm::vec2>(glm::vec2{0.f, 0.f});
					component.Scale() = component_node["Scale"].as<glm::vec2>(glm::vec2{1.f, 1.f});
					component.Size() = component_node["Size"].as<glm::vec2>(glm::vec2{64.f, 64.f});
					component.Rotation() = component_node["Rotation"].as<float>(0.f);
				}
			}
		}
	}

	return true;
}

Sowa::Entity Scene::GetEntityInGroup(const std::string &group) {
	auto view = m_Registry.view<Component::Group>();
	for (auto it = view.rbegin(); it != view.rend(); ++it) {
		Sowa::Entity e(*it, &m_Registry);

		auto groups = e.GetComponent<Component::Group>().Groups();
		for (std::string &g : groups) {
			if (g == group)
				return e;
		}
	}
	return Entity(entt::null, nullptr);
}

std::vector<Sowa::Entity> Scene::GetEntitiesInGroup(const std::string &group) {
	std::vector<Sowa::Entity> entities;
	auto view = m_Registry.view<Component::Group>();
	for (auto it = view.rbegin(); it != view.rend(); ++it) {
		Sowa::Entity e(*it, &m_Registry);

		auto groups = e.GetComponent<Component::Group>().Groups();
		for (std::string &g : groups) {
			if (g == group)
				entities.push_back(e);
		}
	}
	return entities;
}

Sowa::Entity Scene::GetEntityByID(uint32_t id) {
	if (m_Registry.valid((entt::entity)id))
		return Sowa::Entity((entt::entity)id, &m_Registry);

	return Sowa::Entity(entt::null, nullptr);
}
} // namespace Sowa