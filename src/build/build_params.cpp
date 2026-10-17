#include "build/build_params.hpp"

#include <algorithm>
#include <cerrno>
#include <charconv>
#include <cmath>
#include <cstdlib>
#include <system_error>

#include <fmt/format.h>

#include "build/glob.hpp"

namespace sowa::build {

const char *ParamTypeName(ParamType type) {
	switch (type) {
	case ParamType::Bool:
		return "Bool";
	case ParamType::Int:
		return "Int";
	case ParamType::Float:
		return "Float";
	case ParamType::String:
		return "String";
	case ParamType::Enum:
		return "Enum";
	case ParamType::Path:
		return "Path";
	}
	return "?";
}

std::string ParamValueToString(const ParamValue &value) {
	if (std::holds_alternative<bool>(value)) {
		return std::get<bool>(value) ? "true" : "false";
	}
	if (std::holds_alternative<std::int64_t>(value)) {
		return std::to_string(std::get<std::int64_t>(value));
	}
	if (std::holds_alternative<double>(value)) {
		return fmt::format("{}", std::get<double>(value));
	}
	if (std::holds_alternative<std::string>(value)) {
		return std::get<std::string>(value);
	}
	return "nil";
}

bool CheckParamValue(const ParamDecl &decl, const ParamValue &value, std::string &error) {
	double number = 0.0;
	if (std::holds_alternative<std::int64_t>(value)) {
		number = static_cast<double>(std::get<std::int64_t>(value));
	} else if (std::holds_alternative<double>(value)) {
		number = std::get<double>(value);
	}
	if (decl.type == ParamType::Int || decl.type == ParamType::Float) {
		if (decl.min && number < *decl.min) {
			error = fmt::format("{} is below the minimum {}", ParamValueToString(value), *decl.min);
			return false;
		}
		if (decl.max && number > *decl.max) {
			error = fmt::format("{} is above the maximum {}", ParamValueToString(value), *decl.max);
			return false;
		}
	}
	if (decl.type == ParamType::Enum) {
		const std::string &text = std::get<std::string>(value);
		if (std::find(decl.choices.begin(), decl.choices.end(), text) == decl.choices.end()) {
			std::string choices;
			for (const std::string &choice : decl.choices) {
				choices += choices.empty() ? choice : ", " + choice;
			}
			error = "'" + text + "' is not one of: " + choices;
			return false;
		}
	}
	return true;
}

std::optional<ParamValue> ParseParamValue(const ParamDecl &decl, std::string_view text,
	const std::filesystem::path &projectRoot, std::string &error) {
	ParamValue value;
	switch (decl.type) {
	case ParamType::Bool:
		if (text == "true") {
			value = true;
		} else if (text == "false") {
			value = false;
		} else {
			error = "expected true or false, got '" + std::string(text) + "'";
			return std::nullopt;
		}
		break;
	case ParamType::Int: {
		std::int64_t parsed = 0;
		const auto [end, ec] = std::from_chars(text.data(), text.data() + text.size(), parsed);
		if (ec != std::errc() || end != text.data() + text.size() || text.empty()) {
			error = "expected an integer, got '" + std::string(text) + "'";
			return std::nullopt;
		}
		value = parsed;
		break;
	}
	case ParamType::Float: {
		const std::string copy(text);
		char *end = nullptr;
		errno = 0;
		const double parsed = std::strtod(copy.c_str(), &end);
		if (copy.empty() || end != copy.c_str() + copy.size() || errno != 0 ||
			!std::isfinite(parsed)) {
			error = "expected a number, got '" + copy + "'";
			return std::nullopt;
		}
		value = parsed;
		break;
	}
	case ParamType::String:
	case ParamType::Enum:
		value = std::string(text);
		break;
	case ParamType::Path: {
		const auto normalized = NormalizeBuildPath(text, error);
		if (!normalized) {
			return std::nullopt;
		}
		std::error_code ec;
		if (!std::filesystem::exists(projectRoot / *normalized, ec)) {
			error = "'" + *normalized + "' does not exist in the project";
			return std::nullopt;
		}
		value = *normalized;
		break;
	}
	}
	if (!CheckParamValue(decl, value, error)) {
		return std::nullopt;
	}
	return value;
}

std::optional<std::vector<std::pair<std::string, ParamValue>>> ResolveParams(
	const std::vector<ParamDecl> &decls,
	const std::vector<std::pair<std::string, std::string>> &values,
	const std::vector<std::pair<std::string, std::string>> &overrides,
	const std::filesystem::path &projectRoot, std::string &error) {
	const auto findDecl = [&](const std::string &name) {
		return std::find_if(decls.begin(), decls.end(),
			[&](const ParamDecl &decl) { return decl.name == name; });
	};
	for (const auto *list : {&values, &overrides}) {
		for (const auto &[name, text] : *list) {
			(void)text;
			if (findDecl(name) == decls.end()) {
				error = "unknown param '" + name + "' (not declared in build.params)";
				return std::nullopt;
			}
		}
	}

	std::vector<std::pair<std::string, ParamValue>> resolved;
	for (const ParamDecl &decl : decls) {
		const std::string *text = nullptr;
		for (const auto *list : {&values, &overrides}) {
			for (const auto &[name, value] : *list) {
				if (name == decl.name) {
					text = &value;
				}
			}
		}
		if (text != nullptr) {
			std::string problem;
			auto value = ParseParamValue(decl, *text, projectRoot, problem);
			if (!value) {
				error = "param '" + decl.name + "': " + problem;
				return std::nullopt;
			}
			resolved.emplace_back(decl.name, std::move(*value));
		} else if (decl.required) {
			error = "param '" + decl.name + "' is required";
			return std::nullopt;
		} else {
			resolved.emplace_back(decl.name, decl.defaultValue);
		}
	}
	return resolved;
}

} // namespace sowa::build
