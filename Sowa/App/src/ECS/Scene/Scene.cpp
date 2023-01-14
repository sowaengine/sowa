#include "ECS/Scene/Scene.hpp"
#include "Core/Application.hpp"
#include "Debug.hpp"
#include "ECS/Components/Components.hpp"
#include "Resource/ResourceLoader.hpp"
#include "Resource/ResourceManager.hpp"
#include "Utils/File.hpp"
#include "Utils/YAML.hpp"
// #include "box2d/box2d.h"
#include "yaml-cpp/yaml.h"

#include "stlpch.hpp"

#define CALL_FOR_ALL_COMPONENTS(func, ...)               \
	do {                                                 \
		func<Sowa::Component::Camera2D>(__VA_ARGS__);    \
		func<Sowa::Component::Sprite2D>(__VA_ARGS__);    \
		func<Sowa::Component::Transform2D>(__VA_ARGS__); \
	} while (0)

namespace Sowa {
void Scene::StartScene() {
	m_SceneOldCamera2D.camera2d = m_SceneCamera2D.camera2d;
	m_SceneOldCamera2D.transform2d = m_SceneCamera2D.transform2d;
	m_SceneCamera2D.transform2d = Sowa::Component::Transform2D();
	m_SceneCamera2D.camera2d = Sowa::Component::Camera2D();
}

void Scene::StopScene() {
	m_SceneCamera2D.camera2d = m_SceneOldCamera2D.camera2d;
	m_SceneCamera2D.transform2d = m_SceneOldCamera2D.transform2d;
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

	CopyComponent<Component::Sprite2D>(entity, copyEntity);
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
	CopyComponent<Component::Sprite2D>(copyEntity, entity);
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
				if (YAML::Node component_node = components["Camera2D"]; component_node) {
					Component::Camera2D &component = entity.AddComponent<Component::Camera2D>();
					component.Current() = component_node["Current"].as<bool>(false);
					component.Rotatable() = component_node["Rotatable"].as<bool>(true);
					component.Zoom() = component_node["Zoom"].as<float>(1.f);
					component.Center() = component_node["Center"].as<Vec2>(Vec2{0.5f, 0.5f});
				}
				if (YAML::Node component_node = components["Sprite2D"]; component_node) {
					Component::Sprite2D &component = entity.AddComponent<Component::Sprite2D>();
					ResourceID id = component_node["Texture"].as<ResourceID>(0);
					if (id != 0)
						component.Texture() = GetResourceManager<Sowa::ImageTexture>().GetResource(id);
					component.Visible() = component_node["Visible"].as<bool>(true);
				}
				if (YAML::Node component_node = components["Transform2D"]; component_node) {
					Component::Transform2D &component = entity.AddComponent<Component::Transform2D>();
					component.Position() = component_node["Position"].as<glm::vec2>(glm::vec2{0.f, 0.f});
					component.Scale() = component_node["Scale"].as<glm::vec2>(glm::vec2{1.f, 1.f});
					component.Rotation() = component_node["Rotation"].as<float>(0.f);
					// component.ZIndex() = component_node["ZIndex"].as<int>(0);
				}
			}
		}
	}

	return true;
}

Sowa::Entity Scene::GetEntityByID(uint32_t id) {
	if (m_Registry.valid((entt::entity)id))
		return Sowa::Entity((entt::entity)id, &m_Registry);

	return Sowa::Entity(entt::null, nullptr);
}
} // namespace Sowa