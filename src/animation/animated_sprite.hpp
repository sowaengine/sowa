#pragma once

#include "ecs/components/animated_sprite.hpp"
#include "ecs/entity.hpp"
#include "resource/resource_registry.hpp"

namespace sowa {
class World;
void AdvanceAnimatedSprite(AnimatedSprite &sprite, ResourceRegistry &resources, f64 delta);
bool PlayAnimatedSprite(Entity entity, ResourceRegistry &resources, const std::string &name);
void StopAnimatedSprite(Entity entity);
void RegisterAnimatedSpriteSystem(World &world, ResourceRegistry &resources);
} // namespace sowa
