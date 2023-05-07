#ifndef _E_SOWA_HPP__
#define _E_SOWA_HPP__
#pragma once

#include "stlpch.hpp"
#define STR_(x) #x
#define STR(x) STR_(x)

#ifndef SOWA_VERSION_MAJOR
    #define SOWA_VERSION_MAJOR
#endif
#ifndef SOWA_VERSION_MINOR
    #define SOWA_VERSION_MINOR
#endif
#ifndef SOWA_VERSION_PATCH
    #define SOWA_VERSION_PATCH
#endif
#ifndef SOWA_VERSION_TAG
    #define SOWA_VERSION_TAG
#endif
#ifndef SOWA_VERSION_TAG_REV
    #define SOWA_VERSION_TAG_REV
#endif

#ifndef SOWA_VERSION
    #define SOWA_VERSION ""
#endif
#ifndef SOWA_VERSION_NUMBER
    #define SOWA_VERSION_NUMBER ""
#endif


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

namespace sowa {

// Singleton Servers
enum class Server : uint32_t {
	NONE = 0,
	APPLICATION,
	PROJECT,
    SCRIPT_SERVER,
    AUDIO_SERVER
};

#define FILE_BUFFER_CHAR(f) reinterpret_cast<char *>(f.data()), f.size()
#define FILE_BUFFER(f) (f.data()), f.size()

} // namespace sowa

#endif // _E_SOWA_HPP__