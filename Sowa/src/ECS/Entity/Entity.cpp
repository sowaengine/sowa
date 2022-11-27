#include "ECS/Entity/Entity.hpp"

namespace Sowa {
Entity::Entity() {
}

Entity::Entity(entt::entity entityID, entt::registry *pRegistry) {
	SetEntityID(entityID);
	SetRegistry(pRegistry);
}

Entity::~Entity() {
}
} // namespace Sowa