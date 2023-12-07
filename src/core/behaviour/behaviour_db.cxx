#include "behaviour_db.hxx"

#include <iostream>

#include "scene/node_db.hxx"
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

Behaviour BehaviourDB::Construct(std::string name, size_t node_type) {
	Behaviour behaviour;

	size_t id = GetBehaviourID(name);

	if (id == 0 || m_behaviours.find(id) == m_behaviours.end()) {
		Utils::Error("Behaviour {} not found", name);
		return behaviour;
	}

	Behaviour src = m_behaviours[id];
	if (!NodeDB::get().does_inherit(node_type, NodeDB::get().get_node_type(src.extends))) {
		Utils::Error("Behaviour {} expects node type {}. {} does not inherit {}", name, src.extends, NodeDB::get().get_node_typename(node_type), src.extends);
		return behaviour;
	}

	behaviour.m_startFunc = src.m_startFunc;
	behaviour.m_updateFunc = src.m_updateFunc;
	behaviour.m_behaviourID = src.m_behaviourID;
	behaviour.extends = src.extends;

	return behaviour;
}

size_t BehaviourDB::GetBehaviourID(std::string name) {
	return m_behaviourIDs[name];
}
std::string BehaviourDB::GetBehaviourName(size_t id) {
	return m_behaviourNames[id];
}