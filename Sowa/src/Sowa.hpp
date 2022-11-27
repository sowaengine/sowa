#ifndef _E_SOWA_HPP__
#define _E_SOWA_HPP__
#pragma once

#include <functional>
#include <iostream>
#include <memory>
#include <stdlib.h>
#include <string>
#include <unordered_map>

#include "ECS/Entity/Entity.hpp"

#define SOWA_VERSION_MAJOR 0
#define SOWA_VERSION_MINOR 1
#define SOWA_VERSION_PATCH 0

#define SOWA_VERSION_STRING "0.1.0"

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

#define WORLD_TO_SCREEN(x) (x * 25.f)
#define SCREEN_TO_WORLD(x) (x / 25.f)

#ifndef PI
#define PI 3.1416f
#endif
#ifndef DEG2RAD
#define DEG2RAD (PI / 180.0f)
#endif
#ifndef RAD2DEG
#define RAD2DEG (180.0f / PI)
#endif

template <typename T>
using Reference = std::shared_ptr<T>;

typedef uint64_t ResourceID;
typedef uint32_t EntityID;

namespace Sowa {

// Singleton Servers
enum class Server : uint32_t {
	NONE = 0,
	GUISERVER, // Editor Gui Server
	APPLICATION,
	PROJECTSETTINGS,
	SCRIPTSERVER_LUA,
};

} // namespace Sowa

#endif // _E_SOWA_HPP__