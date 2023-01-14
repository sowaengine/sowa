#include "ECS/Scene/Scene.hpp"
#include "Core/Application.hpp"
#include "Debug.hpp"
#include "ECS/Components/Components.hpp"
#include "Resource/ResourceLoader.hpp"
#include "Resource/ResourceManager.hpp"
#include "Utils/File.hpp"

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

bool Scene::Save() {
	return SaveToFile(path.string().c_str());
}
bool Scene::SaveToFile(const char *file) {
	assert(false && "Scene::SaveToFile not implemented");
	return false;
}

bool Scene::LoadFromFile(const char *file) {
	m_Registry.clear();

	ClearResourceManager<Sowa::ImageTexture>();

	std::filesystem::path inpath = Sowa::File::Path(file);
	path = file;
	assert(false && "Scene::LoadFromFile not implemented");
	return false;
}

Sowa::Entity Scene::GetEntityByID(uint32_t id) {
	if (m_Registry.valid((entt::entity)id))
		return Sowa::Entity((entt::entity)id, &m_Registry);

	return Sowa::Entity(entt::null, nullptr);
}
} // namespace Sowa