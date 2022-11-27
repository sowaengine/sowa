#ifndef _E_SCENE_HPP__
#define _E_SCENE_HPP__

#pragma once

#include "ECS/Components/Camera2D/Camera2D.hpp"
#include "ECS/Components/Transform2D/Transform2D.hpp"
#include "ECS/Entity/Entity.hpp"
#include "Resource/ResourceManager.hpp"
#include "Utils/Math.hpp"
#include "entt/entt.hpp"
#include <filesystem>
#include <vector>

class b2World;
namespace Ease {
class Scene {
  public:
	Scene();
	~Scene();

	Entity Create(const std::string &name, EntityID id = 0);
	void Destroy(Ease::Entity &entity);

	entt::registry m_Registry;

	bool Save();
	bool SaveToFile(const char *file);
	bool LoadFromFile(const char *file);
	std::filesystem::path path;

	// Returns first entity that is in given group
	Ease::Entity GetEntityInGroup(const std::string &group);
	std::vector<Ease::Entity> GetEntitiesInGroup(const std::string &group);

	Ease::Entity GetEntityByID(uint32_t id);

	void StartScene();
	void StopScene();

	// Copies all entities of src to dst
	static void CopyScene(Scene &src, Scene &dst);

	void CopyEntity(Ease::Entity entity);
	void AddCopiedEntity(Ease::Entity entity);
	void ClearCopiedEntities();
	Ease::Entity PasteCopiedEntity();
	std::vector<Ease::Entity> PasteCopiedEntities();
	uint32_t GetCopiedEntityCount();

	Vec2 &Gravity() { return m_Gravity; }
	std::shared_ptr<b2World> PhysicsWorld2D() { return m_pPhysicsWorld2D; }

	Ease::Component::Camera2D &CurrentCamera2D() { return m_SceneCamera2D.camera2d; };
	Ease::Component::Transform2D &CurrentCameraTransform2D() { return m_SceneCamera2D.transform2d; }

	template <typename T>
	ResourceManager<T> &GetResourceManager() {
		static ResourceManager<T> manager;
		return manager;
	}

	template <typename T>
	void ClearResourceManager() {
		auto &resources = GetResourceManager<T>();
		resources.GetResources().clear();
	}

  private:
	/**
	 * @brief Registry that holds copied entities
	 */
	entt::registry m_CopyRegistry;

	Vec2 m_Gravity{0.f, -10.f};
	std::shared_ptr<b2World> m_pPhysicsWorld2D;

	struct {
		Ease::Component::Camera2D camera2d;
		Ease::Component::Transform2D transform2d;
	} m_SceneCamera2D{};
	struct {
		Ease::Component::Camera2D camera2d;
		Ease::Component::Transform2D transform2d;
	} m_SceneOldCamera2D{};
};
} // namespace Ease

#endif