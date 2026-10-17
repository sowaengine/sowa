#pragma once

#include <cstddef>
#include <unordered_map>
#include <vector>

#include "core/types.hpp"
#include "ecs/entity.hpp"

namespace sowa {

// Maps per-frame GPU tokens to full, generation-aware ECS handles.
// Token 0 means background; truncating entity IDs could alias recycled entities.
class EntityPickMap {
  public:
	void BeginFrame();
	u32 TokenFor(Entity entity);
	Entity Resolve(u32 token) const;

	size_t Size() const {
		return _entities.size();
	}

  private:
	std::vector<Entity> _entities;
	std::unordered_map<Entity, u32> _tokens;
};

} // namespace sowa
