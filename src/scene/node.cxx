#include "node.hxx"

#include <iostream>

#include "core/behaviour/behaviour_db.hxx"

void Node::AddBehaviour(std::string name) {
	size_t id = BehaviourDB::GetInstance().GetBehaviourID(name);
	if (id == 0)
		return;

	m_behaviours[id] = BehaviourDB::GetInstance().Construct(name);
}
void Node::RemoveBehaviour(std::string name) {
	size_t id = BehaviourDB::GetInstance().GetBehaviourID(name);
	if (id == 0)
		return;

	m_behaviours[id] = Behaviour();
}

void Node::UpdateBehaviours() {
	for (auto &[id, behaviour] : m_behaviours) {
		behaviour.Update(this);
	}
}