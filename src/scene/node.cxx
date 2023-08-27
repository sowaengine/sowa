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

void Node::AddChild(Node *child) {
	if (child->m_parent != nullptr) {
		child->m_parent->RemoveChild(child->Name());
	}

	m_children.push_back(child);
	child->m_parent = this;
}

Node *Node::GetChild(std::string name) {
	for (auto *child : m_children) {
		if (child->Name() == name) {
			return child;
		}
	}

	return nullptr;
}

Node *Node::GetChildIndex(size_t index) {
	if (m_children.size() <= index)
		return nullptr;
	return m_children[index];
}

void Node::RemoveChild(std::string name) {
	for (auto it = m_children.begin(); it != m_children.end(); ++it) {
		if ((*it)->Name() == name) {
			m_children.erase(it);
			return;
		}
	}
}