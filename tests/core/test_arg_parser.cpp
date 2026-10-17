#include <doctest/doctest.h>

#include <filesystem>
#include <fstream>
#include <vector>

#include "core/arg_parser.hpp"

using namespace sowa;

namespace {
ArgParseResult parse(ArgParser &parser, std::initializer_list<std::string> args) {
	std::vector<std::string> strings(args);
	std::vector<char *> argv;
	for (auto &arg : strings) {
		argv.push_back(arg.data());
	}
	return parser.Parse(static_cast<int>(argv.size()), argv.data());
}

struct ExportDirectory {
	std::filesystem::path previous = std::filesystem::current_path();
	std::filesystem::path root = std::filesystem::temp_directory_path() / "sowa_export_args_test";

	ExportDirectory() {
		std::filesystem::remove_all(root);
		std::filesystem::create_directories(root);
		std::filesystem::current_path(root);
	}

	~ExportDirectory() {
		std::filesystem::current_path(previous);
		std::filesystem::remove_all(root);
	}
};
} // namespace

TEST_CASE("Export startup chooses executable-named ZIP and respects explicit project paths") {
	ExportDirectory directory;
	ArgParser parser(true, directory.root);
	REQUIRE(parse(parser, {"/bin/My.Game", "run"}) == ArgParseResult::Continue);
	CHECK(parser.ProjectDirectory() == ".");
	CHECK_FALSE(parser.ProjectIsZip());
	std::ofstream("My.Game.zip") << "archive";
	REQUIRE(parse(parser, {"/bin/My.Game", "run"}) == ArgParseResult::Continue);
	CHECK(parser.ProjectIsZip());
	CHECK(std::filesystem::path(parser.ProjectDirectory()).filename() == "My.Game.zip");
	REQUIRE(parse(parser, {"/bin/My.Game.exe", "run"}) == ArgParseResult::Continue);
	CHECK(parser.ProjectIsZip());
	REQUIRE(parse(parser, {"My.Game", "run", "."}) == ArgParseResult::Continue);
	CHECK_FALSE(parser.ProjectIsZip());
	REQUIRE(parse(parser, {"My.Game", "edit"}) == ArgParseResult::Continue);
	CHECK(parser.Mode() == AppMode::ProjectManager);
	CHECK_FALSE(parser.ProjectIsZip());
	REQUIRE(parse(parser, {"sowa", "run", "other.zip"}) == ArgParseResult::Continue);
	CHECK(parser.Mode() == AppMode::Run);
	CHECK(parser.ProjectIsZip());
	CHECK(parser.ProjectDirectory() == "other.zip");
	CHECK(parse(parser, {"sowa", "edit", "other.zip"}) == ArgParseResult::ExitFailure);
	std::filesystem::create_directory("folder.zip");
	REQUIRE(parse(parser, {"sowa", "edit", "folder.zip"}) == ArgParseResult::Continue);
	CHECK_FALSE(parser.ProjectIsZip());
}

TEST_CASE("project manager opens explicitly and always when no arguments are supplied") {
	ExportDirectory directory;
	ArgParser parser(true, directory.root);
	REQUIRE(parse(parser, {"sowa", "project-manager"}) == ArgParseResult::Continue);
	CHECK(parser.Mode() == AppMode::ProjectManager);
	CHECK(parse(parser, {"sowa", "project-manager", "extra"}) == ArgParseResult::ExitFailure);
	REQUIRE(parse(parser, {"sowa"}) == ArgParseResult::Continue);
	CHECK(parser.Mode() == AppMode::ProjectManager);
	std::ofstream("project.sowa") << "application: {}";
	std::ofstream("sowa.zip") << "archive";
	REQUIRE(parse(parser, {"sowa"}) == ArgParseResult::Continue);
	CHECK(parser.Mode() == AppMode::ProjectManager);
	CHECK_FALSE(parser.ProjectIsZip());
	CHECK(parser.ProjectDirectory() == ".");
	REQUIRE(parse(parser, {"sowa", "edit"}) == ArgParseResult::Continue);
	CHECK(parser.Mode() == AppMode::Edit);
	REQUIRE(parse(parser, {"sowa", "run"}) == ArgParseResult::Continue);
	CHECK(parser.Mode() == AppMode::Run);
	CHECK(parser.ProjectIsZip());
	std::filesystem::remove("sowa.zip");
	REQUIRE(parse(parser, {"sowa"}) == ArgParseResult::Continue);
	CHECK(parser.Mode() == AppMode::ProjectManager);
}

TEST_CASE("export templates only run games and default to the executable-named ZIP") {
	ExportDirectory directory;
	ArgParser parser(false, directory.root);
	REQUIRE(parse(parser, {"/bin/My.Game"}) == ArgParseResult::Continue);
	CHECK(parser.Mode() == AppMode::Run);
	CHECK(parser.ProjectDirectory() == ".");
	CHECK_FALSE(parser.ProjectIsZip());
	std::ofstream("My.Game.zip") << "archive";
	REQUIRE(parse(parser, {"/bin/My.Game"}) == ArgParseResult::Continue);
	CHECK(parser.Mode() == AppMode::Run);
	CHECK(parser.ProjectIsZip());
	CHECK(std::filesystem::path(parser.ProjectDirectory()).filename() == "My.Game.zip");
	REQUIRE(parse(parser, {"My.Game", "run", "game.zip"}) == ArgParseResult::Continue);
	CHECK(parser.ProjectIsZip());
	CHECK(parser.ProjectDirectory() == "game.zip");
	CHECK(parse(parser, {"My.Game", "edit", "."}) == ArgParseResult::ExitFailure);
	CHECK(parse(parser, {"My.Game", "project-manager"}) == ArgParseResult::ExitFailure);
}

TEST_CASE("exported games look for their ZIP next to the executable, not the working directory") {
	ExportDirectory directory;
	std::filesystem::create_directories(directory.root / "game");
	std::ofstream(directory.root / "game" / "My.Game.zip") << "archive";
	std::ofstream("My.Game.zip") << "archive in the working directory";

	ArgParser parser(false, directory.root / "game");
	REQUIRE(parse(parser, {"My.Game"}) == ArgParseResult::Continue);
	CHECK(parser.ProjectIsZip());
	CHECK(std::filesystem::path(parser.ProjectDirectory()) ==
		  directory.root / "game" / "My.Game.zip");

	std::filesystem::remove(directory.root / "game" / "My.Game.zip");
	REQUIRE(parse(parser, {"My.Game"}) == ArgParseResult::Continue);
	CHECK_FALSE(parser.ProjectIsZip());
	CHECK(parser.ProjectDirectory() == ".");
}

TEST_CASE("build command parses targets, params and templates") {
	ExportDirectory directory;
	ArgParser parser(true, directory.root);

	REQUIRE(parse(parser, {"sowa", "build", "game", "--target", "linux", "--target", "web",
							  "--param", "channel=beta", "--param", "note=a=b", "--templates",
							  "tpl"}) == ArgParseResult::Continue);
	CHECK(parser.Mode() == AppMode::Build);
	CHECK(parser.ProjectDirectory() == "game");
	CHECK(parser.Build().targets == std::vector<std::string>{"linux", "web"});
	CHECK_FALSE(parser.Build().all);
	REQUIRE(parser.Build().params.size() == 2);
	CHECK(parser.Build().params[0] == std::pair<std::string, std::string>{"channel", "beta"});
	CHECK(parser.Build().params[1] == std::pair<std::string, std::string>{"note", "a=b"});
	CHECK(parser.Build().templatesDirectory == "tpl");

	REQUIRE(parse(parser, {"sowa", "build", "--all"}) == ArgParseResult::Continue);
	CHECK(parser.ProjectDirectory() == ".");
	CHECK(parser.Build().all);
	CHECK(parser.Build().targets.empty());
	CHECK(parser.Build().params.empty());

	REQUIRE(parse(parser, {"sowa", "build", "game", "--list"}) == ArgParseResult::Continue);
	CHECK(parser.Build().list);
}

TEST_CASE("build command rejects incomplete or conflicting options") {
	ExportDirectory directory;
	ArgParser parser(true, directory.root);
	CHECK(parse(parser, {"sowa", "build"}) == ArgParseResult::ExitFailure);
	CHECK(parse(parser, {"sowa", "build", "--all", "--target", "web"}) ==
		  ArgParseResult::ExitFailure);
	CHECK(parse(parser, {"sowa", "build", "--target"}) == ArgParseResult::ExitFailure);
	CHECK(parse(parser, {"sowa", "build", "--all", "--param", "novalue"}) ==
		  ArgParseResult::ExitFailure);
	CHECK(
		parse(parser, {"sowa", "build", "--all", "--param", "=x"}) == ArgParseResult::ExitFailure);
	CHECK(parse(parser, {"sowa", "build", "--list", "--all"}) == ArgParseResult::ExitFailure);
	CHECK(parse(parser, {"sowa", "build", "a", "b", "--all"}) == ArgParseResult::ExitFailure);
	CHECK(parse(parser, {"sowa", "build", "--all", "--fast"}) == ArgParseResult::ExitFailure);

	ArgParser templateParser(false, directory.root);
	CHECK(parse(templateParser, {"Game", "build", "--all"}) == ArgParseResult::ExitFailure);
}

TEST_CASE("exported macOS apps find their ZIP in the bundle's Resources directory") {
	ExportDirectory directory;
	const auto contents = directory.root / "My Game.app" / "Contents";
	std::filesystem::create_directories(contents / "MacOS");
	std::filesystem::create_directories(contents / "Resources");
	std::ofstream(contents / "Resources" / "My Game.zip") << "archive";

	ArgParser parser(false, contents / "MacOS");
	REQUIRE(parse(parser, {"My Game", "-psn_0_12345"}) == ArgParseResult::Continue);
	CHECK(parser.Mode() == AppMode::Run);
	CHECK(parser.ProjectIsZip());
	CHECK(
		std::filesystem::path(parser.ProjectDirectory()) == contents / "Resources" / "My Game.zip");
}

TEST_CASE("run and edit take debugging options anywhere after the command") {
	ExportDirectory directory;
	std::filesystem::create_directories(directory.root / "game");
	std::ofstream(directory.root / "game" / "project.sowa") << "Application: {}\n";
	ArgParser parser(true, directory.root);

	REQUIRE(parse(parser, {"sowa", "edit", "--wait-for-debugger", "game", "--debug-port",
							  "7000"}) == ArgParseResult::Continue);
	CHECK(parser.Mode() == AppMode::Edit);
	CHECK(parser.ProjectDirectory() == "game");
	CHECK(parser.Debug().waitForDebugger);
	REQUIRE(parser.Debug().port.has_value());
	CHECK(*parser.Debug().port == 7000);

	REQUIRE(parse(parser, {"sowa", "run", "game"}) == ArgParseResult::Continue);
	CHECK_FALSE(parser.Debug().waitForDebugger);
	CHECK_FALSE(parser.Debug().port.has_value());

	CHECK(parse(parser, {"sowa", "run", "game", "--debug-port"}) == ArgParseResult::ExitFailure);
	CHECK(
		parse(parser, {"sowa", "run", "game", "--debug-port", "0"}) == ArgParseResult::ExitFailure);
	CHECK(parse(parser, {"sowa", "run", "game", "--debug-port", "70000"}) ==
		  ArgParseResult::ExitFailure);
	CHECK(parse(parser, {"sowa", "run", "game", "--debug-port", "12x"}) ==
		  ArgParseResult::ExitFailure);
}

TEST_CASE("export templates take debugging options without the run command") {
	ExportDirectory directory;
	ArgParser parser(false, directory.root);
	REQUIRE(parse(parser, {"game", "--wait-for-debugger"}) == ArgParseResult::Continue);
	CHECK(parser.Mode() == AppMode::Run);
	CHECK(parser.Debug().waitForDebugger);
	CHECK(parser.ProjectDirectory() == ".");
}

TEST_CASE("run and edit reject unknown options instead of treating them as project paths") {
	ArgParser parser;
	for (const auto &command : {"run", "edit"}) {
		CHECK(parse(parser, {"sowa", command, "--unknown"}) == ArgParseResult::ExitFailure);
		CHECK(parse(parser, {"sowa", command, "-x"}) == ArgParseResult::ExitFailure);
	}
}

TEST_CASE("debug ports require unsigned decimal digits within the port range") {
	ArgParser parser;
	for (const auto &port :
		{"", " 6510", "+6510", "-1", "6510 ", "0", "65536", "999999999999999999999999999999"}) {
		CHECK(parse(parser, {"sowa", "run", "--debug-port", port}) == ArgParseResult::ExitFailure);
	}
	for (const auto &port : {"1", "65535"}) {
		REQUIRE(
			parse(parser, {"sowa", "run", ".", "--debug-port", port}) == ArgParseResult::Continue);
		CHECK(parser.Debug().port.has_value());
	}
}

TEST_CASE("build options reject empty values and missing values before another option") {
	ArgParser parser;
	for (const auto &option : {"--target", "--param", "--templates"}) {
		CHECK(parse(parser, {"sowa", "build", option, ""}) == ArgParseResult::ExitFailure);
		CHECK(parse(parser, {"sowa", "build", option, "--all"}) == ArgParseResult::ExitFailure);
		CHECK(parse(parser, {"sowa", "build", "--all", option, ""}) == ArgParseResult::ExitFailure);
	}
}
