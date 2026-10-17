#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace sowa::build {

enum class ParamType {
	Bool,
	Int,
	Float,
	String,
	Enum,
	Path,
};

const char *ParamTypeName(ParamType type);

// nil (monostate), bool, Int, Float, or a string for String/Enum/Path.
using ParamValue = std::variant<std::monostate, bool, std::int64_t, double, std::string>;

std::string ParamValueToString(const ParamValue &value);

// One entry of build.params { name = Param.X { ... } }.
struct ParamDecl {
	std::string name;
	ParamType type = ParamType::String;
	ParamValue defaultValue;
	bool required = false;
	std::string description;
	std::optional<double> min;
	std::optional<double> max;
	// Enum only: allowed values, in declaration order.
	std::vector<std::string> choices;
};

// Parses command-line / export_targets.sowa text for decl, enforcing
// type, limits, enum membership, and (Path) that the file exists inside
// projectRoot. Returns nullopt with error set on failure.
std::optional<ParamValue> ParseParamValue(const ParamDecl &decl, std::string_view text,
	const std::filesystem::path &projectRoot, std::string &error);

// Checks a value that already has the right C++ type (a declaration's
// default) against the limits and enum choices.
bool CheckParamValue(const ParamDecl &decl, const ParamValue &value, std::string &error);

// Resolved params for one target: every declared param, in declaration
// order. Values come from the target, then overrides (CLI --param), then
// the declared default. Unknown names, bad values and missing required
// params fail with error naming the param.
std::optional<std::vector<std::pair<std::string, ParamValue>>> ResolveParams(
	const std::vector<ParamDecl> &decls,
	const std::vector<std::pair<std::string, std::string>> &values,
	const std::vector<std::pair<std::string, std::string>> &overrides,
	const std::filesystem::path &projectRoot, std::string &error);

} // namespace sowa::build
