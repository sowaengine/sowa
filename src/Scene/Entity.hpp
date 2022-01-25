/**
 * @file Entity.hpp
 * @author Lexographics
 * @brief 
 * @version 0.1
 * @date 2022-01-26
 */
#ifndef ENTITY_H
#define ENTITY_H
#pragma once

#include "entt/entt.hpp"
#include "Scene.hpp"

class Entity  
{
	private:
		entt::registry* m_pRegistry = nullptr;
		entt::entity m_EntityID;

	public:
      Entity() = default;
		Entity(entt::entity entityID, entt::registry* registry);
		~Entity();

      uint32_t ID() { return (uint32_t)m_EntityID; }

      void SetEntityID(entt::entity entityID) { m_EntityID = entityID; }
      void SetRegistry(entt::registry* pRegistry) { m_pRegistry = pRegistry; }

      bool IsValid() { return m_pRegistry != nullptr && m_pRegistry->valid(m_EntityID); }

      bool operator==(const Entity& other) { return /*m_pRegistry == other.m_pRegistry &&*/ m_EntityID == other.m_EntityID; }
      bool operator==(const Entity& other) const { return /*m_pRegistry == other.m_pRegistry &&*/ m_EntityID == other.m_EntityID; }

		template<typename T, typename... Args>
      T& addComponent(Args&&... args)
      {
         return m_pRegistry->emplace<T>(m_EntityID, std::forward<Args>(args)...);
      }

      template<typename T>
      T& getComponent()
      {
         return m_pRegistry->get<T>(m_EntityID);
      }

      template<typename T>
      void removeComponent()
      {
         m_pRegistry->remove<T>(m_EntityID);
      }

      template<typename T>
      bool hasComponent()
      {
         auto comp = m_pRegistry->try_get<T>(m_EntityID);
         return comp != NULL;
      }		
};
#endif