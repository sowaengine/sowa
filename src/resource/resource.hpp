#pragma once

#include <vector>

#include "core/reflection/field_type.hpp"
#include "core/serialize/archive.hpp"
#include "core/types.hpp"
#include "resource/resource_context.hpp"
#include "resource/resource_id.hpp"

namespace sowa {

// Registry-created asset with reflected fields for serialization, inspector, and Lua.
// Override Serialize for custom formats; call the base to include reflected fields.
class Resource : public Serializable {
  public:
	~Resource() override = default;

	// Assigned when the registry takes ownership; invalid before that.
	ResourceID Id() const {
		return _id;
	}

	// The type's described fields, in registration order. Empty for a resource
	// built outside the registry. This is what the inspector and the script
	// bindings enumerate.
	const std::vector<FieldInfo> &Fields() const;

	void Serialize(Archive &archive) override;

	// Rebuilds derived state from the resource fields, including during hot reload.
	// Must tolerate repeated calls and release previously acquired state.
	virtual void Reload(const ResourceContext &context) {
		(void)context;
	}

	// Releases derived state while the registry context is alive. Called by Remove, Clear,
	// and registry destruction rather than the resource destructor: a retained Ref can
	// outlive the registry and its graphics device.
	virtual void Unload(const ResourceContext &context) {
		(void)context;
	}

  private:
	friend class ResourceRegistry;

	ResourceID _id;

	// Shared reflection metadata stays alive even if the resource outlives its registry.
	Ref<const std::vector<FieldInfo>> _fields;
};

} // namespace sowa
