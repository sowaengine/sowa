#pragma once

#include <string>
#include <vector>

#include "ecs/entity.hpp"
#include "resource/resource.hpp"
#include "resource/resource_id.hpp"

namespace sowa {

class ResourceRegistry;
class World;

// Scene template holding a parsed document and retained sub-resources. Instantiate
// builds an entity subtree without acquiring resource ownership; resources stay alive
// while the template is loaded, including between bursts of instances.
// Reload reads path and retains dependencies; Unload releases them.
class PackedScene : public Resource {
  public:
	std::string path;

	bool RefreshInstance(World &world, ResourceRegistry &resources, Entity instance) const;

	bool IsLoaded() const {
		return !_document.empty();
	}

	void Reload(const ResourceContext &context) override;
	void Unload(const ResourceContext &context) override;

	// Builds editable entities without acquiring resource ownership. During systems,
	// parenting stays deferred until the system ends. Invalid without a loaded root document.
	Entity Instantiate(World &world, ResourceRegistry &resources, Entity parent = Entity()) const;

  private:
	u64 _revision = 0;
	std::string _document;
	std::vector<ResourceID> _retained;
};

} // namespace sowa
