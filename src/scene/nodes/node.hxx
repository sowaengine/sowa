#ifndef SW_NODE_HXX
#define SW_NODE_HXX
#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "core/behaviour/behaviour.hxx"

class Scene;

class Node {
  public:
	inline std::string &name() { return m_name; }
	size_t id() { return m_id; }

	virtual ~Node() = default;

	virtual void _start() {}
	virtual void _update() {}
	virtual void _exit() {}

	inline size_t type_hash() { return m_type_hash; }

	void add_behaviour(std::string name);
	bool has_behaviour(std::string name);
	void remove_behaviour(std::string name);
	void reload_behaviours();
	void start_behaviours();
	void update_behaviours();
	inline std::unordered_map<size_t, Behaviour> &get_behaviours() { return m_behaviours; }
	inline std::vector<std::string> &get_behaviour_names() { return m_behaviour_names; }

	//
	inline std::vector<Node *> &get_children() { return m_children; }
	inline Node *get_parent() { return m_parent; }

	void add_child(Node *child);
	Node *get_child(std::string name);
	inline size_t get_child_count() { return m_children.size(); }
	Node *get_child_index(size_t index);
	// void remove_child(std::string name);
	void remove_child(size_t id);

	Node *duplicate();
	/// @brief Node type specific implementation of node property duplication
	virtual void _duplicate_data(Node *dst) {}

	//
	inline std::vector<std::string> &get_groups() { return m_groups; }
	bool is_in_group(const std::string &name);
	void remove_group(const std::string &name);
	void add_group(const std::string &name);

  protected:
	std::string m_name = "";
	size_t m_id = 0;

  private:
	friend class NodeDB;
	friend class Scene;

	void register_behaviour(const std::string &behaviour, bool callStart = false);
	Node *duplicate_in_scene(Scene *scene);

	size_t m_type_hash = 0;

	std::vector<std::string> m_behaviour_names;
	std::unordered_map<size_t, Behaviour> m_behaviours;

	Node *m_parent = nullptr;
	std::vector<Node *> m_children;
	Scene *m_scene = nullptr;

	std::vector<std::string> m_groups;
};

#endif // SW_NODE_HXX