#include "behaviour_db.hxx"

#include <iostream>

#include "utils/utils.hxx"

BehaviourDB &BehaviourDB::get() {
	static BehaviourDB *db = new BehaviourDB;
	return *db;
}

void BehaviourDB::RegisterBehaviour(std::string name, Behaviour behaviour) {
	static size_t idCounter = 0;
	size_t id;
	if (m_behaviourIDs.find(name) != m_behaviourIDs.end()) {
		id = m_behaviourIDs[name];
	} else {
		id = ++idCounter;
	}

	behaviour.m_behaviourID = id;
	m_behaviours[id] = behaviour;

	m_behaviourNames[id] = name;
	m_behaviourIDs[name] = id;
}

Behaviour BehaviourDB::Construct(std::string name) {
	Behaviour behaviour;

	size_t id = GetBehaviourID(name);

	if (id == 0 || m_behaviours.find(id) == m_behaviours.end()) {
		return behaviour;
	}

	Behaviour src = m_behaviours[id];

	behaviour.m_startFunc = src.m_startFunc;
	behaviour.m_updateFunc = src.m_updateFunc;
	behaviour.m_behaviourID = src.m_behaviourID;

	return behaviour;
}

size_t BehaviourDB::GetBehaviourID(std::string name) {
	return m_behaviourIDs[name];
}
std::string BehaviourDB::GetBehaviourName(size_t id) {
	return m_behaviourNames[id];
}