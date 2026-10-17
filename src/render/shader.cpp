#include "render/shader.hpp"

#include <cctype>
#include <string>

#include "core/filesystem/file_system.hpp"
#include "core/log.hpp"

namespace sowa {
namespace {

std::string bytesToString(const FileData &data) {
	return std::string(reinterpret_cast<const char *>(data.Data()), data.Size());
}

std::string_view trimLeft(std::string_view s) {
	while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) {
		s.remove_prefix(1);
	}
	return s;
}

std::string_view trimRight(std::string_view s) {
	while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back()))) {
		s.remove_suffix(1);
	}
	return s;
}

enum class ShaderStage {
	None,
	Vertex,
	Fragment,
};

ShaderStage parseStageName(std::string_view name) {
	name = trimRight(trimLeft(name));
	if (name == "vertex") {
		return ShaderStage::Vertex;
	}
	if (name == "fragment") {
		return ShaderStage::Fragment;
	}
	return ShaderStage::None;
}

// Returns true if line (already left-trimmed) is a #shader <stage>
// directive. On success, writes the stage into outStage.
bool tryParseShaderDirective(std::string_view line, ShaderStage &outStage) {
	static constexpr std::string_view kPrefix = "#shader";
	if (line.size() < kPrefix.size() || line.substr(0, kPrefix.size()) != kPrefix) {
		return false;
	}

	std::string_view rest = line.substr(kPrefix.size());
	if (!rest.empty() && !std::isspace(static_cast<unsigned char>(rest.front()))) {
		// e.g. #shaderSomething - not a directive
		return false;
	}

	outStage = parseStageName(rest);
	return true;
}

} // namespace

bool ParseShaderSource(std::string_view source, ParsedShaderSource &out) {
	out = {};

	ShaderStage current = ShaderStage::None;
	std::string vertex;
	std::string fragment;
	bool sawVertex = false;
	bool sawFragment = false;

	std::size_t lineStart = 0;
	while (lineStart <= source.size()) {
		const std::size_t lineEnd = source.find('\n', lineStart);
		const std::size_t count =
			(lineEnd == std::string_view::npos ? source.size() : lineEnd) - lineStart;
		std::string_view line = source.substr(lineStart, count);
		if (!line.empty() && line.back() == '\r') {
			line.remove_suffix(1);
		}

		ShaderStage directiveStage = ShaderStage::None;
		if (tryParseShaderDirective(trimLeft(line), directiveStage)) {
			if (directiveStage == ShaderStage::None) {
				Log::Error("ParseShaderSource: unknown stage in `#shader` "
						   "directive: {}",
					line);
				return false;
			}
			if (directiveStage == ShaderStage::Vertex) {
				if (sawVertex) {
					Log::Error("ParseShaderSource: duplicate `#shader vertex` section");
					return false;
				}
				sawVertex = true;
			} else {
				if (sawFragment) {
					Log::Error("ParseShaderSource: duplicate `#shader "
							   "fragment` section");
					return false;
				}
				sawFragment = true;
			}
			current = directiveStage;
		} else if (current == ShaderStage::Vertex) {
			vertex.append(line);
			vertex.push_back('\n');
		} else if (current == ShaderStage::Fragment) {
			fragment.append(line);
			fragment.push_back('\n');
		}

		if (lineEnd == std::string_view::npos) {
			break;
		}
		lineStart = lineEnd + 1;
	}

	if (!sawVertex || !sawFragment) {
		Log::Error("ParseShaderSource: expected both `#shader vertex` and "
				   "`#shader fragment` sections");
		return false;
	}

	out.vertex = std::move(vertex);
	out.fragment = std::move(fragment);
	return true;
}

ShaderHandle CreateShaderFromSource(GraphicsDevice &device, std::string_view source) {
	ParsedShaderSource parsed;
	if (!ParseShaderSource(source, parsed)) {
		return {};
	}
	return device.CreateShader(parsed.vertex, parsed.fragment);
}

ShaderHandle CreateShaderFromFile(GraphicsDevice &device, FileSystem &fs, std::string_view path) {
	const auto file = fs.Load(path);
	if (!file) {
		Log::Error("CreateShaderFromFile: failed to load {} ({})", path,
			static_cast<int>(file.error));
		return {};
	}

	return CreateShaderFromSource(device, bytesToString(*file.value));
}

} // namespace sowa
