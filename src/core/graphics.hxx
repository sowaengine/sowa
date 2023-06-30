#ifndef SW_GRAPHICS_HXX
#define SW_GRAPHICS_HXX
#pragma once

#define GLFW_INCLUDE_NONE

#define GLSL(x) "#version 300 es\n" #x

#include "rendering/model.hxx"
#include "rendering/model_builder.hxx"
#include "rendering/render_layer.hxx"
#include "rendering/shader.hxx"
#include "rendering/texture.hxx"

#endif // SW_GRAPHICS_HXX