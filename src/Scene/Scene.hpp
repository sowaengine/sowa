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

class Entity;
class Scene  
{
	private:
		entt::registry m_Registry;
	public:
		Scene();
		~Scene();

		// Copies all content of a 'src' scene to 'dst' scene
		static void CopyScene(Scene& src, Scene& dst);


		/**
		 * @brief 
		 * 
		 * @param name 
		 * @param targetID create it with a specific id 
		 * @return Entity 
		 */
		Entity Create(const std::string& name, int targetID = -1);

		entt::registry& GetRegistry() { return m_Registry; }


		// Clears all content of the scene (entities)
		void ClearScene();
};
#endif