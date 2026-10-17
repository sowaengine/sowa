#pragma once

// Prevent glfw3.h from pulling in its own GL headers. glad (native) or the
// GLES3 headers (Emscripten) must come first instead.
#define GLFW_INCLUDE_NONE

#if defined(__EMSCRIPTEN__)
#include <GLES3/gl3.h>
#include <GLFW/glfw3.h>
#else
#include <glad/glad.h>

#include <GLFW/glfw3.h>
#endif
