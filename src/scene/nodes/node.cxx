#include "node.hxx"

#include <iostream>

#include "core/app.hxx"
#include "core/behaviour/behaviour_db.hxx"

void Node::add_behaviour(std::string name) {
	if (std::find(m_behaviour_names.begin(), m_behaviour_names.end(), name) != m_behaviour_names.end())
		return;

	m_behaviour_names.push_back(name);
	if (App::get().IsRunning()) {
		register_behaviour(name, true);
	}
}

void Node::remove_behaviour(std::string name) {
	size_t id = BehaviourDB::get().GetBehaviourID(name);
	if (id == 0)
		return;

	m_behaviours[id] = Behaviour();
}

void Node::reload_behaviours() {
	m_behaviours.clear();
	for (const auto &name : m_behaviour_names) {
		register_behaviour(name, true);
	}
}

void Node::start_behaviours() {
	// Reload behaviours without calling _start
	m_behaviours.clear();
	for (const auto &name : m_behaviour_names) {
		register_behaviour(name, false);
	}

	for (auto &[id, behaviour] : m_behaviours) {
		behaviour.Start(this);
	}
}

void Node::update_behaviours() {
	for (auto &[id, behaviour] : m_behaviours) {
		behaviour.Update(this);
	}
}

void Node::add_child(Node *child) {
	if (child->m_parent != nullptr) {
		child->m_parent->remove_child(child->name());
	}

	m_children.push_back(child);
	child->m_parent = this;
}

Node *Node::get_child(std::string name) {
	for (auto *child : m_children) {
		if (child->name() == name) {
			return child;
		}
	}

	return nullptr;
}

Node *Node::get_child_index(size_t index) {
	if (m_children.size() <= index)
		return nullptr;
	return m_children[index];
}

void Node::remove_child(std::string name) {
	for (auto it = m_children.begin(); it != m_children.end(); ++it) {
		if ((*it)->name() == name) {
			m_children.erase(it);
			// (*it)->m_parent = nullptr;
			return;
		}
	}
}

void Node::remove_child(size_t id) {
	for (auto it = m_children.begin(); it != m_children.end(); ++it) {
		if ((*it)->id() == id) {
			m_children.erase(it);
			// (*it)->m_parent = nullptr;
			return;
		}
	}
}

// if callStart, sets data["m_noStart"] = true, then calls _start()
void Node::register_behaviour(const std::string &behaviour, bool callStart) {
	size_t id = BehaviourDB::get().GetBehaviourID(behaviour);
	if (id == 0)
		return;

	m_behaviours[id] = BehaviourDB::get().Construct(behaviour);
	if (callStart) {
		m_behaviours[id].DataTable()["m_noStart"] = true;
		m_behaviours[id].Start(this);
	}
}