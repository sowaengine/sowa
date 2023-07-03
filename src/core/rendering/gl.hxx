#ifndef SW_GL_HXX
#define SW_GL_HXX
#pragma once

#define GLFW_INCLUDE_NONE

#ifdef SW_WEB
#include <GLES3/gl3.h>
#include <GLFW/glfw3.h>
#include <emscripten.h>
#else
#include <glad/glad.h>
//
#include <GLFW/glfw3.h>
#endif

#endif // SW_GL_HXX