#include "ecs/entity_pick_map.hpp"

#include <limits>

namespace sowa {

void EntityPickMap::BeginFrame() {
	_entities.clear();
	_tokens.clear();
	_entities.emplace_back();
}

u32 EntityPickMap::TokenFor(Entity entity) {
	if (!entity.IsAlive()) {
		return 0;
	}
	if (_entities.empty()) {
		BeginFrame();
	}

	const auto found = _tokens.find(entity);
	if (found != _tokens.end()) {
		return found->second;
	}
	if (_entities.size() >= std::numeric_limits<u32>::max()) {
		return 0;
	}

	const u32 token = static_cast<u32>(_entities.size());
	_entities.push_back(entity);
	_tokens.emplace(entity, token);
	return token;
}

Entity EntityPickMap::Resolve(u32 token) const {
	if (token == 0 || token >= _entities.size()) {
		return {};
	}
	const Entity entity = _entities[token];
	return entity.IsAlive() ? entity : Entity{};
}

} // namespace sowa
