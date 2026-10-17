#pragma once

#include <string>
#include <string_view>

#include "render/graphics_device.hpp"

namespace sowa {

class FileSystem;

// Result of splitting a combined .glsl file that uses #shader <stage>
// section markers (vertex / fragment).
struct ParsedShaderSource {
	std::string vertex;
	std::string fragment;
};

// Parses a combined shader source. Expects both #shader vertex and
// #shader fragment sections. Returns false (and logs) on failure.
bool ParseShaderSource(std::string_view source, ParsedShaderSource &out);

ShaderHandle CreateShaderFromSource(GraphicsDevice &device, std::string_view source);

// Loads a combined .glsl via FileSystem (e.g. res://shaders/sprite.glsl),
// parses #shader sections, and compiles through the device. Returns an
// invalid handle (and logs) on failure.
ShaderHandle CreateShaderFromFile(GraphicsDevice &device, FileSystem &fs, std::string_view path);

} // namespace sowa
