#ifndef SW_SCENE_HXX
#define SW_SCENE_HXX
#pragma once

#include <unordered_map>
#include <vector>

#include "camera_2d.hxx"
#include "core/error/error.hxx"
#include "node.hxx"
#include "node_db.hxx"
#include "resource/resource.hxx"
#include "resource/resource_type.hxx"

class Scene {
  public:
	inline std::vector<Node *> &Nodes() { return m_nodes; }

	virtual void BeginScene();
	virtual void UpdateScene();
	virtual void EndScene();

	Error Load(const char *path);
	Error Save(const char *path);

	Node *New(NodeType type, const std::string &name = "", size_t id = 0);
	Resource *LoadResource(const std::string &path, RID id = 0, ResourceType type = ResourceType_None);

	inline const std::vector<RID> &SceneResources() { return m_resources; }

	inline void set_active_camera2d(size_t id) { m_activeCamera2D = id; }
	Camera2D *get_active_camera2d();

	Node *get_node_in_group(std::string group);
	Node *get_node_by_id(size_t id);
	inline const std::string &Path() { return m_path; }

	static void copy(Scene *src, Scene *dst);

  private:
	std::unordered_map<size_t, Node *> m_allocatedNodes;
	std::vector<RID> m_resources;

	std::vector<Node *> m_nodes;
	size_t m_activeCamera2D;

	std::string m_path = "";
};

#endif // SW_SCENE_HXX