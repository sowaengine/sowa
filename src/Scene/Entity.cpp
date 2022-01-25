/**
 * @file Entity.cpp
 * @author Lexographics
 * @brief 
 * @version 0.1
 * @date 2022-01-26
 */
#include "Entity.hpp"  
	
Entity::Entity(entt::entity entityID, entt::registry* registry)
   : m_pRegistry(registry), m_EntityID(entityID)
{
}
	
Entity::~Entity()
{
	
}