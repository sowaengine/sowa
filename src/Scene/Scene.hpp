/**
 * @file Scene.hpp
 * @author Lexographics
 * @brief 
 * @version 0.1
 * @date 2022-01-26
 */
#ifndef SCENE_H
#define SCENE_H
#pragma once

#include "entt/entt.hpp"
#include "Entity.hpp"

class Scene  
{
	private:
		entt::registry m_Registry;

		std::string m_Name;
	public:
		Scene();
		~Scene();

		// Copies all content of a 'src' scene to 'dst' scene
		static void CopyScene(Scene& src, Scene& dst);

		static Entity GetEntityByName(const std::string& name);

		static bool SaveScene(const std::string& path, Scene& scene);
		static bool LoadScene(const std::string& path, Scene& scene);

		/**
		 * @brief 
		 * 
		 * @param name 
		 * @param targetID create it with a specific id 
		 * @return Entity 
		 */
		Entity Create(const std::string& name, int targetID = -1, uint32_t targetUUID = 0);

		entt::registry& GetRegistry() { return m_Registry; }


		// Clears all content of the scene (entities)
		void ClearScene();


		std::string GetName() { return m_Name; }
		void SetName(const std::string& name) { m_Name = name; }
};
#endif