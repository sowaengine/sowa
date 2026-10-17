#pragma once

#include <vector>

#include "resource/resource_id.hpp"

namespace sowa {

// Resource ownership stored on a scene root. Unregistered so the entity serializer
// skips it; records are written in the top-level resources block.
// Insertion order keeps scene saves stable.
struct SceneResources {
	std::vector<ResourceID> owned;
};

} // namespace sowa
