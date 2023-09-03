#include "node.hxx"

#include <iostream>

#include "core/app.hxx"
#include "core/behaviour/behaviour_db.hxx"

void Node::AddBehaviour(std::string name) {
	if (std::find(m_behaviourNames.begin(), m_behaviourNames.end(), name) != m_behaviourNames.end())
		return;

	m_behaviourNames.push_back(name);
	if (App::GetInstance().IsRunning()) {
		register_behaviour(name, true);
	}
}

void Node::RemoveBehaviour(std::string name) {
	size_t id = BehaviourDB::GetInstance().GetBehaviourID(name);
	if (id == 0)
		return;

	m_behaviours[id] = Behaviour();
}

void Node::ReloadBehaviours() {
	m_behaviours.clear();
	for (const auto &name : m_behaviourNames) {
		register_behaviour(name, true);
	}
}

void Node::StartBehaviours() {
	// Reload behaviours without calling _start
	m_behaviours.clear();
	for (const auto &name : m_behaviourNames) {
		register_behaviour(name, false);
	}

	for (auto &[id, behaviour] : m_behaviours) {
		behaviour.Start(this);
	}
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

// if callStart, sets data["m_noStart"] = true, then calls _start()
void Node::register_behaviour(const std::string &behaviour, bool callStart) {
	size_t id = BehaviourDB::GetInstance().GetBehaviourID(behaviour);
	if (id == 0)
		return;

	m_behaviours[id] = BehaviourDB::GetInstance().Construct(behaviour);
	if (callStart) {
		m_behaviours[id].DataTable()["m_noStart"] = true;
		m_behaviours[id].Start(this);
	}
}