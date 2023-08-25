#ifndef SW_BEHAVIOUR_DB_HXX
#define SW_BEHAVIOUR_DB_HXX
#pragma once

#include <functional>
#include <string>
#include <unordered_map>

#include "behaviour.hxx"

class BehaviourDB {
  public:
	static BehaviourDB &GetInstance();

	void RegisterBehaviour(std::string name, Behaviour behaviour);
	Behaviour Construct(std::string name);

	size_t GetBehaviourID(std::string name);
	std::string GetBehaviourName(size_t id);

  private:
	std::unordered_map<size_t, Behaviour> m_behaviours;
	std::unordered_map<size_t, std::string> m_behaviourNames;
	std::unordered_map<std::string, size_t> m_behaviourIDs;
};

#endif // SW_BEHAVIOUR_DB_HXX