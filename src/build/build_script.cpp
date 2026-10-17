#include "build/build_script.hpp"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>

#include <sol/sol.hpp>

#include "build/export_targets.hpp"
#include "core/build_info.hpp"
#include "core/log.hpp"
#include "render/viewport_scale.hpp"
#include "script/lua_bindings.hpp"

namespace sowa::build {
namespace {

// Runs before build.lua: the Platform and Param globals, and helpers the C++
// side uses to build ctx (returned as a table, never global).
constexpr const char *kPrelude = R"lua(
local internal = {}

local function describe(value)
	return math.type(value) or type(value)
end

local function readonly(name, values, keys)
	return setmetatable({}, {
		__index = function(_, key)
			if not keys[key] then
				error(("%s has no field '%s'"):format(name, tostring(key)), 2)
			end
			return values[key]
		end,
		__newindex = function()
			error(("%s is read-only"):format(name), 2)
		end,
		__pairs = function()
			return next, values, nil
		end,
	})
end
internal.readonly = readonly

do
	local values = { Windows = "windows", MacOS = "macos", Linux = "linux", Web = "web" }
	local keys = {}
	for key in pairs(values) do
		keys[key] = true
	end
	Platform = readonly("Platform", values, keys)
end

do
	local param_mt = {}
	internal.param_mt = param_mt
	local order = 0
	local constructors = {}
	local keys = {}
	for _, type_name in ipairs({ "Bool", "Int", "Float", "String", "Enum", "Path" }) do
		keys[type_name] = true
		constructors[type_name] = function(options)
			if options == nil then
				options = {}
			end
			if type(options) ~= "table" then
				error(("Param.%s expects a table, got %s"):format(type_name, describe(options)), 2)
			end
			local decl = {}
			for key, value in pairs(options) do
				decl[key] = value
			end
			order = order + 1
			decl.__type = type_name
			decl.__order = order
			return setmetatable(decl, param_mt)
		end
	end
	Param = readonly("Param", constructors, keys)
end

function internal.project(data, scale_modes, scale_mode_list, state)
	local schema = {
		application = {
			name = "string",
			author = "string",
			version = "string",
			icon = "string",
			main_scene = "string",
			scripts = "list",
			autoloads = "list",
		},
		rendering = {
			window = { width = "integer", height = "integer" },
			viewport = { width = "integer", height = "integer", scale_mode = "scale_mode" },
		},
		global_store = "map",
	}

	local function check_unlocked(path)
		if state.locked then
			error(("ctx.project.%s can only be changed in configure"):format(path), 3)
		end
	end

	local function check(path, kind, value)
		if kind == "string" then
			if type(value) ~= "string" then
				error(("ctx.project.%s must be a string, got %s"):format(path, describe(value)), 3)
			end
			return value
		elseif kind == "integer" then
			local integer = math.tointeger(value)
			if integer == nil then
				error(("ctx.project.%s must be an integer, got %s"):format(path, describe(value)), 3)
			end
			return integer
		elseif kind == "scale_mode" then
			if type(value) ~= "string" or not scale_modes[value] then
				error(("ctx.project.%s must be one of: %s"):format(path, scale_mode_list), 3)
			end
			return value
		elseif kind == "list" then
			if type(value) ~= "table" then
				error(("ctx.project.%s must be a list of strings, got %s"):format(path, describe(value)), 3)
			end
			local copy = {}
			for index, item in ipairs(value) do
				if type(item) ~= "string" then
					error(("ctx.project.%s[%d] must be a string, got %s"):format(path, index, describe(item)), 3)
				end
				copy[index] = item
			end
			return copy
		end
	end

	local function store(path, values)
		return setmetatable({}, {
			__index = function(_, key)
				return values[key]
			end,
			__newindex = function(_, key, value)
				check_unlocked(path)
				if type(key) ~= "string" then
					error(("ctx.project.%s keys must be strings"):format(path), 2)
				end
				if value ~= nil and type(value) ~= "string" then
					error(("ctx.project.%s.%s must be a string, got %s"):format(path, key, describe(value)), 2)
				end
				values[key] = value
			end,
			__pairs = function()
				return next, values, nil
			end,
		})
	end

	local function struct(prefix, fields, values)
		local children = {}
		for key, kind in pairs(fields) do
			if type(kind) == "table" then
				children[key] = struct(prefix .. key .. ".", kind, values[key])
			elseif kind == "map" then
				children[key] = store(prefix .. key, values[key])
			end
		end
		return setmetatable({}, {
			__index = function(_, key)
				if fields[key] == nil then
					error(("unknown project setting 'ctx.project.%s%s'"):format(prefix, tostring(key)), 2)
				end
				return children[key] or values[key]
			end,
			__newindex = function(_, key, value)
				local kind = fields[key]
				if kind == nil then
					error(("unknown project setting 'ctx.project.%s%s'"):format(prefix, tostring(key)), 2)
				end
				check_unlocked(prefix .. key)
				if children[key] then
					error(("cannot replace ctx.project.%s%s; assign its fields instead"):format(prefix, key), 2)
				end
				values[key] = check(prefix .. key, kind, value)
			end,
			__pairs = function()
				local key
				return function()
					key = next(fields, key)
					if key ~= nil then
						return key, children[key] or values[key]
					end
				end
			end,
		})
	end

	return struct("", schema, data)
end

return internal
)lua";

constexpr const char *kDefaultScript = R"lua(
build.platforms { Platform.Windows, Platform.MacOS, Platform.Linux, Platform.Web }
build.on("configure", function(ctx)
	ctx:mount("res"):include("**")
end)
)lua";

const std::set<std::string> kEvents = {"configure", "staged", "done", "failed"};

// Throws a Lua error carrying the calling script position ("build.lua:12: ").
[[noreturn]] void raise(lua_State *L, const std::string &message) {
	luaL_where(L, 1);
	const char *where = lua_tostring(L, -1);
	std::string text = where != nullptr ? where : "";
	lua_pop(L, 1);
	throw std::runtime_error(text + message);
}

// C++ exceptions thrown from bindings become Lua errors carrying just their
// message. sol's default handler also prints each one to stderr, which would
// duplicate every ctx:abort and build.lua mistake.
int quietExceptionHandler(lua_State *L, sol::optional<const std::exception &>,
	sol::string_view description) {
	return sol::stack::push(L, description);
}

std::string describe(const sol::object &object) {
	switch (object.get_type()) {
	case sol::type::lua_nil:
	case sol::type::none:
		return "nil";
	case sol::type::boolean:
		return "boolean";
	case sol::type::number:
		return "number";
	case sol::type::string:
		return "string";
	case sol::type::table:
		return "table";
	case sol::type::function:
		return "function";
	default:
		return "userdata";
	}
}

bool isInteger(lua_State *L, const sol::object &object) {
	if (object.get_type() != sol::type::number) {
		return false;
	}
	object.push(L);
	const bool integer = lua_isinteger(L, -1) != 0;
	lua_pop(L, 1);
	return integer;
}

std::string errorText(const sol::protected_function_result &result) {
	const sol::error error = result;
	std::string text = error.what();
	// Script errors already name the build.lua line; the traceback adds
	// only C frames.
	if (const auto traceback = text.find("\nstack traceback:"); traceback != std::string::npos) {
		text.erase(traceback);
	}
	return text;
}

sol::object toLua(sol::state &lua, const ParamValue &value) {
	if (std::holds_alternative<bool>(value)) {
		return sol::make_object(lua, std::get<bool>(value));
	}
	if (std::holds_alternative<std::int64_t>(value)) {
		return sol::make_object(lua, std::get<std::int64_t>(value));
	}
	if (std::holds_alternative<double>(value)) {
		return sol::make_object(lua, std::get<double>(value));
	}
	if (std::holds_alternative<std::string>(value)) {
		return sol::make_object(lua, std::get<std::string>(value));
	}
	return sol::make_object(lua, sol::lua_nil);
}

bool isParamName(const std::string &name) {
	if (name.empty() || (name[0] >= '0' && name[0] <= '9')) {
		return false;
	}
	return std::all_of(name.begin(), name.end(),
		[](char c) { return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_'; });
}

ParamType paramType(const std::string &name) {
	if (name == "Bool") {
		return ParamType::Bool;
	}
	if (name == "Int") {
		return ParamType::Int;
	}
	if (name == "Float") {
		return ParamType::Float;
	}
	if (name == "Enum") {
		return ParamType::Enum;
	}
	if (name == "Path") {
		return ParamType::Path;
	}
	return ParamType::String;
}

} // namespace

struct BuildScript::Impl {
	sol::state lua;
	sol::table internal;
	std::filesystem::path projectRoot;
	bool hasBuildFile = false;
	bool platformsDeclared = false;
	bool paramsDeclared = false;
	std::vector<ParamDecl> params;
	std::vector<std::string> platforms;
	std::map<std::string, std::vector<sol::protected_function>> handlers;
	std::int64_t timestamp = 0;
	std::map<std::string, sol::object> loadedModules;
	std::vector<std::string> loadingModules;

	// The target whose ctx is live. Closures handed to Lua capture the
	// generation they were made for, so a ctx kept past its target fails
	// cleanly instead of touching a finished target.
	BuildTarget *target = nullptr;
	int generation = 0;
	sol::table ctx;
	sol::table projectData;
	sol::table projectState;
	std::vector<sol::protected_function> writeFunctions;

	BuildTarget &current(lua_State *L, int forGeneration) {
		if (target == nullptr || forGeneration != generation) {
			raise(L, "this ctx belongs to a target that has finished building");
		}
		return *target;
	}

	void bindGlobals();
	void declareParams(lua_State *L, const sol::object &argument);
	void declarePlatforms(lua_State *L, const sol::object &argument);
	sol::object require(lua_State *L, const std::string &name);
	sol::table makeBuilder(int forGeneration, bool output);
	sol::table makeProject(const ProjectSettings &project);
	bool readProject(ProjectSettings &project, std::string &error);
	bool runHandlers(const std::string &event, BuildTarget &target, std::string &error,
		const std::string *message = nullptr);
};

void BuildScript::Impl::bindGlobals() {
	lua.open_libraries(sol::lib::base, sol::lib::string, sol::lib::table, sol::lib::math,
		sol::lib::utf8);
	lua.set_exception_handler(&quietExceptionHandler);
	lua["dofile"] = sol::lua_nil;
	lua["loadfile"] = sol::lua_nil;
	lua["load"] = sol::lua_nil;
	BindLog(lua);
	lua.set_function("print", [](sol::this_state state, sol::variadic_args args) {
		lua_State *L = state;
		std::string line;
		for (int i = 1; i <= lua_gettop(L); ++i) {
			std::size_t length = 0;
			const char *text = luaL_tolstring(L, i, &length);
			if (i > 1) {
				line += '\t';
			}
			line.append(text, length);
			lua_pop(L, 1);
		}
		(void)args;
		Log::Info("{}", line);
	});
	lua.set_function("require",
		[this](sol::this_state state, const std::string &name) { return require(state, name); });

	sol::table build = lua.create_named_table("build");
	build.set_function("params",
		[this](sol::this_state state, sol::object argument) { declareParams(state, argument); });
	build.set_function("platforms",
		[this](sol::this_state state, sol::object argument) { declarePlatforms(state, argument); });
	build.set_function("on", [this](sol::this_state state, sol::object event, sol::object fn) {
		lua_State *L = state;
		if (event.get_type() != sol::type::string || !kEvents.count(event.as<std::string>())) {
			raise(L, "build.on: unknown event; expected configure, staged, done or failed");
		}
		if (fn.get_type() != sol::type::function) {
			raise(L, "build.on: expected a function, got " + describe(fn));
		}
		handlers[event.as<std::string>()].push_back(fn.as<sol::protected_function>());
	});
	build.set_function("timestamp", [this]() { return timestamp; });
	build["engine_version"] = std::string(BuildInfo::Version());

	auto prelude = lua.safe_script(kPrelude, sol::script_pass_on_error, "=build-prelude");
	if (!prelude.valid()) {
		throw std::runtime_error(errorText(prelude));
	}
	internal = prelude.get<sol::table>();
}

void BuildScript::Impl::declareParams(lua_State *L, const sol::object &argument) {
	if (paramsDeclared) {
		raise(L, "build.params can only be called once");
	}
	paramsDeclared = true;
	if (argument.get_type() != sol::type::table) {
		raise(L, "build.params expects a table, got " + describe(argument));
	}
	const sol::table paramMeta = internal["param_mt"];
	std::vector<std::pair<std::int64_t, ParamDecl>> ordered;

	argument.as<sol::table>().for_each([&](const sol::object &key, const sol::object &value) {
		if (key.get_type() != sol::type::string || !isParamName(key.as<std::string>())) {
			raise(L, "build.params: param names must match [a-z_][a-z0-9_]*");
		}
		const std::string name = key.as<std::string>();
		if (value.get_type() != sol::type::table) {
			raise(L, "build.params: '" + name + "' must be a Param.* value");
		}
		sol::table options = value.as<sol::table>();
		sol::optional<sol::table> meta = options[sol::metatable_key];
		if (!meta || *meta != paramMeta) {
			raise(L, "build.params: '" + name + "' must be a Param.* value");
		}

		ParamDecl decl;
		decl.name = name;
		const std::string typeName = options["__type"];
		decl.type = paramType(typeName);
		const std::string where = "param '" + name + "' (Param." + typeName + ")";

		std::optional<sol::object> defaultValue;
		options.for_each([&](const sol::object &field, const sol::object &fieldValue) {
			if (field.get_type() == sol::type::number) {
				if (decl.type != ParamType::Enum || fieldValue.get_type() != sol::type::string) {
					raise(L, where + ": unexpected list entry");
				}
				return;
			}
			const std::string fieldName = field.as<std::string>();
			if (fieldName == "__type" || fieldName == "__order") {
				return;
			}
			if (fieldName == "default") {
				defaultValue = fieldValue;
			} else if (fieldName == "required") {
				if (fieldValue.get_type() != sol::type::boolean) {
					raise(L, where + ": required must be a boolean");
				}
				decl.required = fieldValue.as<bool>();
			} else if (fieldName == "description") {
				if (fieldValue.get_type() != sol::type::string) {
					raise(L, where + ": description must be a string");
				}
				decl.description = fieldValue.as<std::string>();
			} else if ((fieldName == "min" || fieldName == "max") &&
					   (decl.type == ParamType::Int || decl.type == ParamType::Float)) {
				if (fieldValue.get_type() != sol::type::number) {
					raise(L, where + ": " + fieldName + " must be a number");
				}
				(fieldName == "min" ? decl.min : decl.max) = fieldValue.as<double>();
			} else {
				raise(L, where + ": unknown field '" + fieldName + "'");
			}
		});

		if (decl.type == ParamType::Enum) {
			const std::size_t count = options.size();
			for (std::size_t i = 1; i <= count; ++i) {
				const std::string choice = options[i];
				if (std::find(decl.choices.begin(), decl.choices.end(), choice) !=
					decl.choices.end()) {
					raise(L, where + ": '" + choice + "' is listed twice");
				}
				decl.choices.push_back(choice);
			}
			if (decl.choices.empty()) {
				raise(L, where + ": list the allowed values, e.g. Param.Enum { \"a\", \"b\" }");
			}
		}
		if (decl.min && decl.max && *decl.min > *decl.max) {
			raise(L, where + ": min is greater than max");
		}

		if (defaultValue && decl.required) {
			raise(L, where + ": a required param cannot have a default");
		}
		if (defaultValue) {
			const sol::object &value = *defaultValue;
			std::string problem;
			switch (decl.type) {
			case ParamType::Bool:
				if (value.get_type() != sol::type::boolean) {
					raise(L, where + ": default must be a boolean, got " + describe(value));
				}
				decl.defaultValue = value.as<bool>();
				break;
			case ParamType::Int:
				if (!isInteger(L, value)) {
					raise(L, where + ": default must be an integer, got " + describe(value));
				}
				decl.defaultValue = value.as<std::int64_t>();
				break;
			case ParamType::Float:
				if (value.get_type() != sol::type::number) {
					raise(L, where + ": default must be a number, got " + describe(value));
				}
				decl.defaultValue = value.as<double>();
				break;
			case ParamType::String:
			case ParamType::Enum:
				if (value.get_type() != sol::type::string) {
					raise(L, where + ": default must be a string, got " + describe(value));
				}
				decl.defaultValue = value.as<std::string>();
				break;
			case ParamType::Path: {
				if (value.get_type() != sol::type::string) {
					raise(L, where + ": default must be a string, got " + describe(value));
				}
				auto parsed = ParseParamValue(decl, value.as<std::string>(), projectRoot, problem);
				if (!parsed) {
					raise(L, where + ": default " + problem);
				}
				decl.defaultValue = *parsed;
				break;
			}
			}
			if (!CheckParamValue(decl, decl.defaultValue, problem)) {
				raise(L, where + ": default " + problem);
			}
		}
		ordered.emplace_back(options.get<std::int64_t>("__order"), std::move(decl));
	});

	std::sort(ordered.begin(), ordered.end(),
		[](const auto &a, const auto &b) { return a.first < b.first; });
	for (auto &[order, decl] : ordered) {
		(void)order;
		params.push_back(std::move(decl));
	}
}

void BuildScript::Impl::declarePlatforms(lua_State *L, const sol::object &argument) {
	if (platformsDeclared) {
		raise(L, "build.platforms can only be called once");
	}
	platformsDeclared = true;
	if (argument.get_type() != sol::type::table) {
		raise(L, "build.platforms expects a list of Platform values, got " + describe(argument));
	}
	sol::table list = argument.as<sol::table>();
	for (std::size_t i = 1; i <= list.size(); ++i) {
		sol::object item = list[i];
		if (item.get_type() != sol::type::string || !IsKnownPlatform(item.as<std::string>())) {
			raise(L, "build.platforms: entry " + std::to_string(i) +
						 " is not a Platform value (use Platform.Windows, Platform.MacOS, "
						 "Platform.Linux or Platform.Web)");
		}
		const std::string platform = item.as<std::string>();
		if (std::find(platforms.begin(), platforms.end(), platform) != platforms.end()) {
			raise(L, "build.platforms: '" + platform + "' is listed twice");
		}
		platforms.push_back(platform);
	}
	if (platforms.empty()) {
		raise(L, "build.platforms needs at least one platform");
	}
}

sol::object BuildScript::Impl::require(lua_State *L, const std::string &name) {
	std::string path = name;
	if (path.rfind("res://", 0) == 0) {
		path = path.substr(6);
	}
	if (path.size() < 4 || path.substr(path.size() - 4) != ".lua") {
		path += ".lua";
	}
	std::string error;
	const auto normalized = NormalizeBuildPath(path, error);
	if (!normalized) {
		raise(L, "require: " + error);
	}
	if (const auto it = loadedModules.find(*normalized); it != loadedModules.end()) {
		return it->second;
	}
	if (std::find(loadingModules.begin(), loadingModules.end(), *normalized) !=
		loadingModules.end()) {
		raise(L, "require: '" + *normalized + "' requires itself");
	}
	std::ifstream file(projectRoot / *normalized, std::ios::binary);
	if (!file) {
		raise(L, "require: cannot read '" + *normalized + "'");
	}
	std::stringstream text;
	text << file.rdbuf();

	loadingModules.push_back(*normalized);
	auto result = lua.safe_script(text.str(), sol::script_pass_on_error, "@" + *normalized);
	loadingModules.pop_back();
	if (!result.valid()) {
		throw std::runtime_error(errorText(result));
	}
	sol::object value = result.get_type() == sol::type::none ? sol::make_object(lua, true)
															 : result.get<sol::object>();
	if (value.get_type() == sol::type::lua_nil) {
		value = sol::make_object(lua, true);
	}
	loadedModules[*normalized] = value;
	return value;
}

sol::table BuildScript::Impl::makeBuilder(int forGeneration, bool output) {
	sol::table builder = lua.create_table();
	const auto rules = [this, forGeneration, output](lua_State *L) -> std::vector<FileRule> & {
		BuildTarget &target = current(L, forGeneration);
		if (target.phase != BuildPhase::Configure) {
			raise(L, "files can only be added or removed in configure");
		}
		return output ? target.outputRules : target.resRules;
	};
	const auto pattern = [](lua_State *L, const sol::object &object, const char *method) {
		if (object.get_type() != sol::type::string) {
			raise(L, std::string(method) + " expects a pattern string, got " + describe(object));
		}
		std::string error;
		auto parsed = GlobPattern::Parse(object.as<std::string>(), error);
		if (!parsed) {
			raise(L, std::string(method) + ": " + error);
		}
		return *parsed;
	};
	const auto destination = [](lua_State *L, const sol::object &object, const char *method) {
		if (object.get_type() != sol::type::string) {
			raise(L, std::string(method) + " expects a path string, got " + describe(object));
		}
		std::string error;
		auto path = NormalizeBuildPath(object.as<std::string>(), error);
		if (!path) {
			raise(L, std::string(method) + ": " + error);
		}
		return *path;
	};

	builder.set_function("include", [rules, pattern, destination](sol::table self, sol::object glob,
										sol::object options, sol::this_state state) {
		lua_State *L = state;
		FileRule rule;
		rule.kind = FileRule::Kind::Include;
		rule.pattern = pattern(L, glob, "include");
		if (options.get_type() == sol::type::table) {
			options.as<sol::table>().for_each([&](const sol::object &key,
												  const sol::object &value) {
				const std::string name =
					key.get_type() == sol::type::string ? key.as<std::string>() : "?";
				if (name == "as") {
					rule.as = destination(L, value, "include: as");
				} else if (name == "into") {
					if (value.get_type() == sol::type::string && value.as<std::string>() == ".") {
						rule.into = ".";
					} else {
						rule.into = destination(L, value, "include: into");
					}
				} else {
					raise(L, "include: unknown option '" + name + "' (expected as or into)");
				}
			});
		} else if (options.get_type() != sol::type::lua_nil &&
				   options.get_type() != sol::type::none) {
			raise(L, "include: options must be a table, got " + describe(options));
		}
		if (rule.as && rule.into) {
			raise(L, "include: use either as or into, not both");
		}
		if (rule.as && !rule.pattern->IsExplicit()) {
			raise(L, "include: as needs a single file path, not a pattern");
		}
		rules(L).push_back(std::move(rule));
		return self;
	});
	builder.set_function("exclude",
		[rules, pattern](sol::table self, sol::object glob, sol::this_state state) {
			lua_State *L = state;
			FileRule rule;
			rule.kind = FileRule::Kind::Exclude;
			rule.pattern = pattern(L, glob, "exclude");
			rules(L).push_back(std::move(rule));
			return self;
		});
	builder.set_function("write", [rules, destination](sol::table self, sol::object path,
									  sol::object content, sol::this_state state) {
		lua_State *L = state;
		FileRule rule;
		rule.kind = FileRule::Kind::Write;
		rule.path = destination(L, path, "write");
		if (content.get_type() == sol::type::string || content.get_type() == sol::type::number) {
			content.push(L);
			std::size_t length = 0;
			const char *text = lua_tolstring(L, -1, &length);
			rule.content.assign(text, length);
			lua_pop(L, 1);
		} else {
			raise(L, "write expects string content, got " + describe(content));
		}
		rules(L).push_back(std::move(rule));
		return self;
	});
	builder.set_function("write_fn", [this, rules, destination](sol::table self, sol::object path,
										 sol::object fn, sol::this_state state) {
		lua_State *L = state;
		FileRule rule;
		rule.kind = FileRule::Kind::WriteFunction;
		rule.path = destination(L, path, "write_fn");
		if (fn.get_type() != sol::type::function) {
			raise(L, "write_fn expects a function, got " + describe(fn));
		}
		auto &list = rules(L);
		rule.function = static_cast<int>(writeFunctions.size());
		writeFunctions.push_back(fn.as<sol::protected_function>());
		list.push_back(std::move(rule));
		return self;
	});
	builder.set_function("files", [this, forGeneration, output](sol::this_state state) {
		lua_State *L = state;
		BuildTarget &target = current(L, forGeneration);
		std::vector<std::string> warnings;
		std::string error;
		const auto files = ResolveRules(output ? target.outputRules : target.resRules,
			target.resolve, warnings, error);
		if (!files) {
			raise(L, "files: " + error);
		}
		std::vector<std::string> paths;
		for (const ResolvedFile &file : *files) {
			paths.push_back(file.destination);
		}
		if (!output && std::find(paths.begin(), paths.end(), "project.sowa") == paths.end()) {
			paths.insert(std::lower_bound(paths.begin(), paths.end(), "project.sowa"),
				"project.sowa");
		}
		sol::table list = lua.create_table();
		for (std::size_t i = 0; i < paths.size(); ++i) {
			list[i + 1] = paths[i];
		}
		return list;
	});
	return builder;
}

sol::table BuildScript::Impl::makeProject(const ProjectSettings &project) {
	const auto list = [this](const std::vector<std::string> &items) {
		sol::table table = lua.create_table();
		for (std::size_t i = 0; i < items.size(); ++i) {
			table[i + 1] = items[i];
		}
		return table;
	};
	std::string scaleMode;
	sol::table scaleModes = lua.create_table();
	std::string scaleModeList;
	for (const EnumConstant &constant : ViewportScaleModeConstants()) {
		scaleModes[constant.name] = true;
		scaleModeList += scaleModeList.empty() ? constant.name : ", " + constant.name;
		if (constant.value == static_cast<i32>(project.rendering.viewport.scaleMode)) {
			scaleMode = constant.name;
		}
	}

	projectData = lua.create_table_with("application",
		lua.create_table_with("name", project.application.name, "author",
			project.application.author, "version", project.application.version, "icon",
			project.application.icon, "main_scene", project.application.mainScene, "scripts",
			list(project.application.scripts), "autoloads", list(project.application.autoloads)),
		"rendering",
		lua.create_table_with("window",
			lua.create_table_with("width", project.rendering.window.width, "height",
				project.rendering.window.height),
			"viewport",
			lua.create_table_with("width", project.rendering.viewport.width, "height",
				project.rendering.viewport.height, "scale_mode", scaleMode)),
		"global_store", lua.create_table());
	sol::table store = projectData["global_store"];
	for (const auto &[key, value] : project.globalStore.GetStore()) {
		store[key] = value;
	}
	projectState = lua.create_table_with("locked", false);
	sol::protected_function make = internal["project"];
	auto result = make(projectData, scaleModes, scaleModeList, projectState);
	if (!result.valid()) {
		throw std::runtime_error(errorText(result));
	}
	return result.get<sol::table>();
}

bool BuildScript::Impl::readProject(ProjectSettings &project, std::string &error) {
	sol::table application = projectData["application"];
	sol::table rendering = projectData["rendering"];
	sol::table window = rendering["window"];
	sol::table viewport = rendering["viewport"];

	const auto readList = [&](const char *key, std::vector<std::string> &out) {
		sol::table table = application[key];
		std::vector<std::string> items;
		for (std::size_t i = 1; i <= table.size(); ++i) {
			sol::object item = table[i];
			if (item.get_type() != sol::type::string) {
				error = "ctx.project.application." + std::string(key) + "[" + std::to_string(i) +
						"] must be a string, got " + describe(item);
				return false;
			}
			items.push_back(item.as<std::string>());
		}
		out = std::move(items);
		return true;
	};
	const auto readInt = [&](sol::table table, const char *key, const std::string &path, i32 &out) {
		const std::int64_t value = table.get<std::int64_t>(key);
		if (value <= 0 || value > 1 << 20) {
			error = path + " must be between 1 and " + std::to_string(1 << 20);
			return false;
		}
		out = static_cast<i32>(value);
		return true;
	};

	project.application.name = application.get<std::string>("name");
	project.application.author = application.get<std::string>("author");
	project.application.version = application.get<std::string>("version");
	project.application.icon = application.get<std::string>("icon");
	project.application.mainScene = application.get<std::string>("main_scene");
	if (!readList("scripts", project.application.scripts) ||
		!readList("autoloads", project.application.autoloads) ||
		!readInt(window, "width", "ctx.project.rendering.window.width",
			project.rendering.window.width) ||
		!readInt(window, "height", "ctx.project.rendering.window.height",
			project.rendering.window.height) ||
		!readInt(viewport, "width", "ctx.project.rendering.viewport.width",
			project.rendering.viewport.width) ||
		!readInt(viewport, "height", "ctx.project.rendering.viewport.height",
			project.rendering.viewport.height)) {
		return false;
	}
	const std::string scaleMode = viewport.get<std::string>("scale_mode");
	for (const EnumConstant &constant : ViewportScaleModeConstants()) {
		if (constant.name == scaleMode) {
			project.rendering.viewport.scaleMode = static_cast<ViewportScaleMode>(constant.value);
		}
	}
	StringStore store;
	sol::table storeTable = projectData["global_store"];
	storeTable.for_each([&](const sol::object &key, const sol::object &value) {
		store.Set(key.as<std::string>(), value.as<std::string>());
	});
	project.globalStore = std::move(store);
	return true;
}

bool BuildScript::Impl::runHandlers(const std::string &event, BuildTarget &target,
	std::string &error, const std::string *message) {
	const auto it = handlers.find(event);
	if (it == handlers.end()) {
		return true;
	}
	for (sol::protected_function &handler : it->second) {
		auto result = message ? handler(ctx, *message) : handler(ctx);
		if (target.aborted) {
			error = target.abortMessage;
			return false;
		}
		if (!result.valid()) {
			error = errorText(result);
			return false;
		}
	}
	return true;
}

BuildScript::BuildScript() : _impl(std::make_unique<Impl>()) {
}

BuildScript::~BuildScript() = default;

bool BuildScript::Load(const std::filesystem::path &projectRoot, std::string &error) {
	Impl &impl = *_impl;
	impl.projectRoot = projectRoot;
	impl.timestamp = std::chrono::duration_cast<std::chrono::seconds>(
		std::chrono::system_clock::now().time_since_epoch())
						 .count();
	try {
		impl.bindGlobals();
	} catch (const std::exception &e) {
		error = std::string("build script setup failed: ") + e.what();
		return false;
	}

	std::string source = kDefaultScript;
	std::string chunk = "=default build.lua";
	std::error_code ec;
	const auto path = projectRoot / kBuildScriptFile;
	if (std::filesystem::exists(path, ec)) {
		std::ifstream file(path, std::ios::binary);
		if (!file) {
			error = "Cannot read " + path.string();
			return false;
		}
		std::stringstream text;
		text << file.rdbuf();
		source = text.str();
		chunk = "@" + std::string(kBuildScriptFile);
		impl.hasBuildFile = true;
	}

	impl.loadingModules.push_back(std::string(kBuildScriptFile));
	auto result = impl.lua.safe_script(source, sol::script_pass_on_error, chunk);
	impl.loadingModules.clear();
	if (!result.valid()) {
		error = errorText(result);
		return false;
	}
	if (!impl.platformsDeclared) {
		error = "build.lua must call build.platforms { ... }";
		return false;
	}
	return true;
}

bool BuildScript::HasBuildFile() const {
	return _impl->hasBuildFile;
}

const std::vector<ParamDecl> &BuildScript::Params() const {
	return _impl->params;
}

const std::vector<std::string> &BuildScript::Platforms() const {
	return _impl->platforms;
}

std::int64_t BuildScript::Timestamp() const {
	return _impl->timestamp;
}

bool BuildScript::Configure(BuildTarget &target, std::string &error) {
	Impl &impl = *_impl;
	sol::state &lua = impl.lua;
	impl.target = &target;
	const int generation = ++impl.generation;
	impl.writeFunctions.clear();
	target.phase = BuildPhase::Configure;

	try {
		sol::table params = lua.create_table();
		sol::table paramKeys = lua.create_table();
		for (const auto &[name, value] : target.params) {
			params[name] = toLua(lua, value);
			paramKeys[name] = true;
		}
		sol::protected_function readonly = impl.internal["readonly"];

		sol::table resBuilder = impl.makeBuilder(generation, false);
		sol::table outputBuilder = impl.makeBuilder(generation, true);

		sol::table values = lua.create_table();
		values["target"] = target.name;
		values["platform"] = target.platform;
		values["params"] = readonly("ctx.params", params, paramKeys).get<sol::table>();
		values["project"] = impl.makeProject(target.project);
		values["output"] = target.output.string();
		values.set_function("mount",
			[resBuilder](sol::object, sol::object name, sol::this_state state) {
				if (name.get_type() != sol::type::string || name.as<std::string>() != "res") {
					raise(state, "ctx:mount: only \"res\" can be mounted");
				}
				return resBuilder;
			});
		values.set_function("output_dir", [outputBuilder](sol::object) { return outputBuilder; });
		values.set_function("abort", [&impl, generation](sol::object, sol::object message,
										 sol::this_state state) {
			lua_State *L = state;
			BuildTarget &current = impl.current(L, generation);
			std::string text = "build aborted";
			if (message.get_type() != sol::type::lua_nil && message.get_type() != sol::type::none) {
				message.push(L);
				std::size_t length = 0;
				const char *chars = luaL_tolstring(L, -1, &length);
				text.assign(chars, length);
				lua_pop(L, 2);
			}
			current.aborted = true;
			current.abortMessage = text;
			throw std::runtime_error(text);
		});
		sol::table keys = lua.create_table();
		for (const char *key :
			{"target", "platform", "params", "project", "output", "mount", "output_dir", "abort"}) {
			keys[key] = true;
		}
		impl.ctx = readonly("ctx", values, keys).get<sol::table>();
	} catch (const std::exception &e) {
		error = e.what();
		return false;
	}

	if (!impl.runHandlers("configure", target, error)) {
		return false;
	}
	impl.projectState["locked"] = true;
	if (!impl.readProject(target.project, error)) {
		return false;
	}
	target.phase = BuildPhase::Stage;
	return true;
}

bool BuildScript::RunWriteFunction(BuildTarget &target, int index, std::string &bytes,
	std::string &error) {
	Impl &impl = *_impl;
	if (index < 0 || index >= static_cast<int>(impl.writeFunctions.size())) {
		error = "internal error: unknown write_fn";
		return false;
	}
	auto open = std::make_shared<bool>(true);
	std::string *buffer = &bytes;
	sol::table handle = impl.lua.create_table();
	handle.set_function("write", [open, buffer](sol::this_state state, sol::variadic_args) {
		lua_State *L = state;
		if (!*open) {
			raise(L, "this file handle is closed; use it only inside its write_fn");
		}
		for (int i = 2; i <= lua_gettop(L); ++i) {
			const int type = lua_type(L, i);
			if (type != LUA_TSTRING && type != LUA_TNUMBER) {
				raise(L, std::string("f:write expects strings or numbers, got ") +
							 lua_typename(L, type));
			}
			std::size_t length = 0;
			const char *text = luaL_tolstring(L, i, &length);
			buffer->append(text, length);
			lua_pop(L, 1);
		}
	});
	handle.set_function("line", [open, buffer](sol::this_state state, sol::variadic_args) {
		lua_State *L = state;
		if (!*open) {
			raise(L, "this file handle is closed; use it only inside its write_fn");
		}
		if (lua_gettop(L) < 2) {
			buffer->push_back('\n');
			return;
		}
		try {
			buffer->append(FormatLuaArgs(L, 2));
		} catch (const std::exception &e) {
			raise(L, std::string("f:line: ") + e.what());
		}
		buffer->push_back('\n');
	});

	auto result = impl.writeFunctions[index](handle, impl.ctx);
	*open = false;
	if (target.aborted) {
		error = target.abortMessage;
		return false;
	}
	if (!result.valid()) {
		error = errorText(result);
		return false;
	}
	return true;
}

bool BuildScript::Emit(std::string_view event, BuildTarget &target, std::string &error) {
	return _impl->runHandlers(std::string(event), target, error);
}

void BuildScript::EmitFailed(BuildTarget &target, const std::string &message) {
	if (!_impl->ctx.valid() || _impl->target != &target) {
		return;
	}
	target.phase = BuildPhase::Failed;
	// The abort that may have caused this failure is already reported.
	target.aborted = false;
	std::string error;
	if (!_impl->runHandlers("failed", target, error, &message)) {
		Log::Error("build.lua failed handler: {}", error);
	}
}

} // namespace sowa::build
