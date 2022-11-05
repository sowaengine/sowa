#ifndef _E_ENTITY_HPP__
#define _E_ENTITY_HPP__

#pragma once

#include "entt/entt.hpp"

namespace Ease {
class Entity {
  public:
	Entity();
	Entity(entt::entity entityID, entt::registry *pRegistry);
	~Entity();

	uint32_t ID() { return (uint32_t)m_Entity; }

	void SetEntityID(entt::entity entityID) { m_Entity = entityID; }
	void SetRegistry(entt::registry *pRegistry) { m_pRegistry = pRegistry; }

	bool IsValid() { return m_pRegistry != nullptr && m_pRegistry->valid(m_Entity); }

	bool operator==(const Entity &other) { return m_Entity == other.m_Entity; }
	bool operator==(const Entity &other) const { return m_Entity == other.m_Entity; }

	template <typename T, typename... Args>
	T &AddComponent(Args &&...args) {
		return m_pRegistry->emplace<T>(m_Entity, std::forward<Args>(args)...);
	}
	template <typename T>
	T &GetComponent() {
		return m_pRegistry->get<T>(m_Entity);
	}
	template <typename T>
	void RemoveComponent() {
		m_pRegistry->remove<T>(m_Entity);
	}
	template <typename T>
	bool HasComponent() {
		if (!IsValid())
			return false;
		auto comp = m_pRegistry->try_get<T>(m_Entity);
		return comp != NULL;
	}

  private:
	entt::entity m_Entity;
	entt::registry *m_pRegistry = nullptr;
};
} // namespace Ease

#endif