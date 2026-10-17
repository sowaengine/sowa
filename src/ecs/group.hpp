#pragma once

#include <flecs.h>

namespace sowa {

// Cross-cutting labels such as "enemy" or "pausable". Each group is an entity;
// InGroup relationships support queries over all members and listing an entity's groups.
struct InGroup {};

// Separate scope prevents group-name collisions. World::Find excludes it.
inline constexpr const char *kGroupScopeName = "Groups";

} // namespace sowa
