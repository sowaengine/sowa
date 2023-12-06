#ifndef SW_SCENE_HXX
#define SW_SCENE_HXX
#pragma once

#include <unordered_map>
#include <vector>

#include "core/error/error.hxx"
#include "node_db.hxx"
#include "nodes/node.hxx"
#include "resource/resource.hxx"
#include "resource/resource_locker.hxx"
#include "resource/resource_type.hxx"
#include "scene/nodes/2d/camera_2d.hxx"

class Scene {
  public:
	inline Node *&Root() { return m_root; }
	virtual ~Scene() = default;

	void _begin_scene();
	void _update_scene();
	void _end_scene();

	ErrorCode load(const char *path);
	ErrorCode save(const char *path);
	void clear();

	Node *create(NodeType type, const std::string &name = "", size_t id = 0);
	void queue_free(size_t id);
	void free(size_t id);

	Resource *load_resource(const std::string &path, RID id = 0, ResourceType type = ResourceType_None);

	inline const ResourceLocker &scene_resources() { return m_resources; }

	inline void set_active_camera2d(size_t id) { m_active_camera_2d = id; }
	Camera2D *get_active_camera2d();

	Node *get_node_in_group(std::string group);
	Node *get_node_by_id(size_t id);
	inline const std::string &Path() { return m_path; }

	static void copy(Scene *src, Scene *dst);

  private:
	std::unordered_map<size_t, Node *> m_allocated_nodes;
	ResourceLocker m_resources;

	Node *m_root = nullptr;
	size_t m_active_camera_2d;

	std::vector<size_t> m_nodes_to_free;

	std::string m_path = "";
};

#endif // SW_SCENE_HXX