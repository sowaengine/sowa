#include <doctest/doctest.h>

#include <filesystem>
#include <fstream>

#include "core/filesystem/file_system.hpp"
#include "render/graphics_device.hpp"
#include "render/shader.hpp"

#include "render/stub_graphics_device.hpp"

using namespace sowa;

TEST_CASE("ParseShaderSource splits vertex and fragment sections") {
	const char *source = R"(#shader vertex
#version 300 es
void main() {}
#shader fragment
#version 300 es
void main() { }
)";

	ParsedShaderSource parsed;
	REQUIRE(ParseShaderSource(source, parsed));
	CHECK(parsed.vertex.find("#version 300 es") != std::string::npos);
	CHECK(parsed.vertex.find("void main() {}") != std::string::npos);
	CHECK(parsed.fragment.find("#version 300 es") != std::string::npos);
	CHECK(parsed.fragment.find("void main() { }") != std::string::npos);
	CHECK(parsed.vertex.find("#shader") == std::string::npos);
	CHECK(parsed.fragment.find("#shader") == std::string::npos);
}

TEST_CASE("ParseShaderSource rejects missing or duplicate sections") {
	ParsedShaderSource parsed;

	CHECK_FALSE(ParseShaderSource("#shader vertex\nvoid main(){}\n", parsed));
	CHECK_FALSE(ParseShaderSource("#shader fragment\nvoid main(){}\n", parsed));
	CHECK_FALSE(ParseShaderSource(
		"#shader vertex\na\n#shader vertex\nb\n#shader fragment\nc\n", parsed));
	CHECK_FALSE(ParseShaderSource("#shader geometry\nvoid main(){}\n", parsed));
}

TEST_CASE("CreateShaderFromFile fails when the file is missing") {
	const auto root =
		std::filesystem::temp_directory_path() / "sowa_shader_missing_test";
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root);

	FileSystem fs;
	fs.MountFolder("res", root);

	StubGraphicsDevice device;
	const ShaderHandle handle =
		CreateShaderFromFile(device, fs, "res://missing.glsl");

	CHECK_FALSE(handle.IsValid());
	CHECK_FALSE(device.createCalled);

	std::filesystem::remove_all(root);
}

TEST_CASE("CreateShaderFromFile loads, parses, and compiles via the device") {
	const auto root =
		std::filesystem::temp_directory_path() / "sowa_shader_load_test";
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root);
	{
		std::ofstream(root / "sprite.glsl") << R"(#shader vertex
vertex-src
#shader fragment
fragment-src
)";
	}

	FileSystem fs;
	fs.MountFolder("res", root);

	StubGraphicsDevice device;
	const ShaderHandle handle =
		CreateShaderFromFile(device, fs, "res://sprite.glsl");

	CHECK(handle.IsValid());
	CHECK(handle.id == 1);
	CHECK(handle.version == 1);
	CHECK(device.createCalled);
	CHECK(device.lastVertex.find("vertex-src") != std::string::npos);
	CHECK(device.lastFragment.find("fragment-src") != std::string::npos);

	std::filesystem::remove_all(root);
}

TEST_CASE("VertexLayoutStride packs attributes tightly") {
	VertexLayout layout;
	layout.attributes = {
		{0, VertexAttrType::Vec3},
		{1, VertexAttrType::Vec4},
		{2, VertexAttrType::UInt32},
	};
	CHECK(VertexLayoutStride(layout) == sizeof(f32) * 7 + sizeof(u32));
}
