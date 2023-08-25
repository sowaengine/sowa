#include "behaviour.hxx"
#include "behaviour_db.hxx"

#include <iostream>

Behaviour Behaviour::New(BehaviourFunc startFunc, BehaviourFunc updateFunc) {
	Behaviour behaviour;
	behaviour.m_startFunc = startFunc;
	behaviour.m_updateFunc = updateFunc;

	return behaviour;
}

void Behaviour::Start(Node *node) {
	if (m_startFunc)
		m_startFunc(node);
}
void Behaviour::Update(Node *node) {
	if (m_updateFunc)
		m_updateFunc(node);
}

std::string Behaviour::GetBehaviourName() {
	if (m_behaviourID == 0)
		return "";

	return BehaviourDB::GetInstance().GetBehaviourName(m_behaviourID);
}

size_t Behaviour::GetBehaviourID() {
	return m_behaviourID;
}