#include <doctest/doctest.h>

#include <filesystem>
#include <fstream>

#include "build/build_params.hpp"

using namespace sowa::build;

namespace {

ParamDecl decl(const char *name, ParamType type) {
	ParamDecl result;
	result.name = name;
	result.type = type;
	return result;
}

} // namespace

TEST_CASE("param values parse by type and respect limits") {
	const auto root = std::filesystem::temp_directory_path();
	std::string error;

	const ParamDecl flag = decl("server", ParamType::Bool);
	CHECK(std::get<bool>(*ParseParamValue(flag, "true", root, error)));
	CHECK_FALSE(std::get<bool>(*ParseParamValue(flag, "false", root, error)));
	CHECK_FALSE(ParseParamValue(flag, "yes", root, error));

	ParamDecl level = decl("level", ParamType::Int);
	level.min = 0;
	level.max = 4;
	CHECK(std::get<std::int64_t>(*ParseParamValue(level, "3", root, error)) == 3);
	CHECK_FALSE(ParseParamValue(level, "5", root, error));
	CHECK(error.find("maximum") != std::string::npos);
	CHECK_FALSE(ParseParamValue(level, "-1", root, error));
	CHECK_FALSE(ParseParamValue(level, "2.5", root, error));
	CHECK_FALSE(ParseParamValue(level, "", root, error));

	const ParamDecl volume = decl("volume", ParamType::Float);
	CHECK(std::get<double>(*ParseParamValue(volume, "0.5", root, error)) == doctest::Approx(0.5));
	CHECK_FALSE(ParseParamValue(volume, "loud", root, error));

	ParamDecl store = decl("store", ParamType::Enum);
	store.choices = {"none", "steam"};
	CHECK(std::get<std::string>(*ParseParamValue(store, "steam", root, error)) == "steam");
	CHECK_FALSE(ParseParamValue(store, "itch", root, error));
	CHECK(error.find("none, steam") != std::string::npos);
}

TEST_CASE("Path params must exist inside the project") {
	const auto root = std::filesystem::temp_directory_path() / "sowa_build_params_test";
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root / "configs");
	std::ofstream(root / "configs" / "a.yaml") << "x";

	const ParamDecl config = decl("config", ParamType::Path);
	std::string error;
	CHECK(std::get<std::string>(*ParseParamValue(config, "./configs/a.yaml", root, error)) ==
		  "configs/a.yaml");
	CHECK_FALSE(ParseParamValue(config, "configs/b.yaml", root, error));
	CHECK_FALSE(ParseParamValue(config, "../outside", root, error));
	std::filesystem::remove_all(root);
}

TEST_CASE("resolved params apply target values, then overrides, then defaults") {
	const auto root = std::filesystem::temp_directory_path();
	ParamDecl channel = decl("channel", ParamType::Enum);
	channel.choices = {"dev", "beta", "release"};
	channel.required = true;
	ParamDecl demo = decl("demo", ParamType::Bool);
	demo.defaultValue = false;
	const ParamDecl note = decl("note", ParamType::String);
	const std::vector<ParamDecl> decls = {channel, demo, note};

	std::string error;
	auto resolved = ResolveParams(decls, {{"channel", "release"}, {"demo", "true"}},
		{{"channel", "beta"}}, root, error);
	REQUIRE_MESSAGE(resolved, error);
	REQUIRE(resolved->size() == 3);
	CHECK((*resolved)[0].first == "channel");
	CHECK(std::get<std::string>((*resolved)[0].second) == "beta");
	CHECK(std::get<bool>((*resolved)[1].second));
	CHECK(std::holds_alternative<std::monostate>((*resolved)[2].second));

	CHECK_FALSE(ResolveParams(decls, {}, {}, root, error));
	CHECK(error == "param 'channel' is required");
	CHECK_FALSE(ResolveParams(decls, {{"channel", "dev"}}, {{"chanel", "x"}}, root, error));
	CHECK(error.find("unknown param 'chanel'") != std::string::npos);
	CHECK_FALSE(ResolveParams(decls, {{"channel", "nightly"}}, {}, root, error));
	CHECK(error.find("param 'channel'") == 0);
}
