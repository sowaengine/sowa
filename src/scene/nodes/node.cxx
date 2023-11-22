#include "node.hxx"

#include <iostream>

#include "core/app.hxx"
#include "core/behaviour/behaviour_db.hxx"

void Node::add_behaviour(std::string name) {
	if (has_behaviour(name))
		return;

	m_behaviour_names.push_back(name);
	if (App::get().IsRunning()) {
		register_behaviour(name, true);
	}
}

bool Node::has_behaviour(std::string name) {
	return std::find(m_behaviour_names.begin(), m_behaviour_names.end(), name) != m_behaviour_names.end();
}

void Node::remove_behaviour(std::string name) {
	std::vector<std::string>::iterator position = std::find(m_behaviour_names.begin(), m_behaviour_names.end(), name);
	if (position != m_behaviour_names.end())
		m_behaviour_names.erase(position);

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
		child->m_parent->remove_child(child->id());
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

// void Node::remove_child(std::string name) {
// 	for (auto it = m_children.begin(); it != m_children.end(); ++it) {
// 		if ((*it)->name() == name) {
// 			m_children.erase(it);
// 			// (*it)->m_parent = nullptr;
// 			return;
// 		}
// 	}
// }

void Node::remove_child(size_t id) {
	for (auto it = m_children.begin(); it != m_children.end(); ++it) {
		if ((*it)->id() == id) {
			(*it)->m_parent = nullptr;
			m_children.erase(it);
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

bool Node::is_in_group(const std::string &name) {
	return std::find(m_groups.begin(), m_groups.end(), name) != m_groups.end();
}

void Node::remove_group(const std::string &name) {
	std::vector<std::string>::iterator position = std::find(m_groups.begin(), m_groups.end(), name);
	if (position != m_groups.end())
		m_groups.erase(position);
}

void Node::add_group(const std::string &name) {
	if (is_in_group(name))
		return;

	m_groups.push_back(name);
}

Node *Node::duplicate() {
	Node *newNode = duplicate_in_scene(m_scene);
	_duplicate_data(newNode);
	return newNode;
}

Node *Node::duplicate_in_scene(Scene *scene) {
	std::function<Node *(Scene *, Node *)> copyNode;

	copyNode = [&](Scene *scn, Node *node) -> Node * {
		Node *newNode = scn->create(node->type_hash(), node->name(), node->id());
		std::vector<std::string> props;
		NodeDB::get().get_property_names(node->type_hash(), props);
		for (auto &propName : props) {
			NodeProperty prop = NodeDB::get().get_property(node->type_hash(), propName);
			prop.set(newNode, prop.get(node));
		}

		newNode->get_behaviour_names() = node->get_behaviour_names();

		for (Node *child : node->get_children()) {
			newNode->add_child(copyNode(scn, child));
		}

		newNode->get_groups() = node->get_groups();

		return newNode;
	};

	Node *node = copyNode(scene, this);

	// if (App::get().IsRunning()) {
	// 	std::function<void(Node *)> start;
	//
	// 	start = [&](Node *n) {
	// 		n->start_behaviours();
	//
	// 		for (Node *child : n->get_children()) {
	// 			start(child);
	// 		}
	// 	};
	//
	// 	start(node);
	// }
	return node;
}

bool Node::is_visible() {
	if (nullptr != get_parent())
		return m_visible && get_parent()->is_visible();

	return m_visible;
}