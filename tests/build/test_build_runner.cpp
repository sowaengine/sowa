#include <doctest/doctest.h>

#include <filesystem>
#include <fstream>
#include <sstream>

#include "build/build_runner.hpp"
#include "core/filesystem/file_system.hpp"

#include <miniz.h>

using namespace sowa;
using namespace sowa::build;

namespace {

namespace fs = std::filesystem;

void write(const fs::path &path, std::string_view text) {
	fs::create_directories(path.parent_path());
	std::ofstream(path, std::ios::binary) << text;
}

std::string read(const fs::path &path) {
	std::ifstream file(path, std::ios::binary);
	std::stringstream text;
	text << file.rdbuf();
	return text.str();
}

struct BuildFixture {
	fs::path root = fs::temp_directory_path() / "sowa_build_runner_test";
	fs::path project = root / "game";
	fs::path templates = root / "templates";

	BuildFixture() {
		fs::remove_all(root);
		write(project / "project.sowa",
			"Application:\n  Name: My Game\n  Version: 1.2.0\n"
			"Rendering:\n  Window:\n    Width: 1280\n    Height: 720\n"
			"Physics:\n  TicksPerSecond: 120\n");
		write(project / "scenes" / "main.scene", "scene");
		write(project / "scenes" / "world2" / "boss.scene", "boss");
		write(project / "secret" / "key.txt", "key");
		write(project / ".env", "ENV=desktop");
		write(project / ".env.web", "ENV=web");
		write(project / ".data" / "save.txt", "save");
		write(project / "LICENSE.txt", "license");
		write(project / "vendor" / "steam" / "steam_api.dll", "dll");
		write(project / "thirdparty" / "licenses" / "lua" / "LICENSE", "lua license");
		write(project / "icon.png", "png");
		write(templates / "linux" / "sowa", "linux engine");
		write(templates / "windows" / "sowa.exe", "windows engine");
		write(templates / "macos" / "sowa", "mac engine");
		write(templates / "web" / "sowa.html",
			"<html><head><title>Sowa Engine</title></head><body></body></html>");
		write(templates / "web" / "sowa.js", "js");
		write(templates / "web" / "sowa.wasm", "wasm");
	}

	~BuildFixture() {
		fs::remove_all(root);
	}

	void script(std::string_view text) {
		write(project / "build.lua", text);
	}

	void targets(std::string_view text) {
		write(project / "export_targets.sowa", text);
	}

	BuildReport build(std::vector<std::string> names,
		std::vector<std::pair<std::string, std::string>> overrides = {}) {
		BuildRequest request;
		request.projectRoot = project;
		request.targets = std::move(names);
		request.paramOverrides = std::move(overrides);
		request.templatesDirectory = templates;
		return RunBuild(request);
	}
};

// Reads one file out of an exported game archive.
std::string archived(const fs::path &zip, const std::string &path) {
	FileSystem files;
	REQUIRE(files.MountZip("res", zip) == FsError::Ok);
	auto loaded = files.Load("res://" + path);
	if (!loaded || !loaded.value) {
		return "<missing>";
	}
	const auto &bytes = loaded.value->bytes;
	return std::string(reinterpret_cast<const char *>(bytes.data()), bytes.size());
}

constexpr const char *kScript = R"lua(
build.params {
    server  = Param.Bool { default = false },
    channel = Param.Enum { "dev", "beta", "release", required = true },
    demo    = Param.Bool { default = false },
}

build.platforms { Platform.Linux, Platform.Windows, Platform.Web }

build.on("configure", function(ctx)
    if ctx.platform == Platform.Web and ctx.params.server then
        ctx:abort("Can not create a server build on web")
    end

    local res = ctx:mount("res")
        :include("**")
        :exclude("secret/**")
        :exclude(".env")
        :include(".data/**")
        :exclude("vendor/**")
        :exclude("thirdparty/**")
        :write("stamp.txt", tostring(build.timestamp()))

    if ctx.platform == Platform.Web then
        res:include(".env.web", { as = ".env" })
        ctx.project.rendering.window.width = 960
        ctx.project.physics.ticks_per_second = 30
    end
    if ctx.params.demo then
        res:exclude("scenes/world2/**")
        ctx.project.application.name = ctx.project.application.name .. " Demo"
    end
    ctx.project.global_store.channel = ctx.params.channel

    res:write_fn("info.txt", function(f, ctx)
        f:line("{} {} {}", ctx.target, ctx.platform, ctx.project.application.version)
        f:write("files=", #res:files(), "\n")
    end)

    ctx:output_dir()
        :include("LICENSE.txt")
        :include("vendor/steam/*.dll", { into = "." })
        :include("thirdparty/licenses/**", { into = "licenses" })
end)
)lua";

constexpr const char *kTargets = R"(
Targets:
  - Name: linux
    Platform: linux
    Params: { channel: release }
  - Name: demo
    Platform: windows
    Params: { channel: beta, demo: true }
  - Name: web
    Platform: web
    Output: ../site
    Params: { channel: release }
)";

} // namespace

TEST_CASE("export_targets.sowa round-trips and validates names and platforms") {
	BuildFixture fixture;
	ExportTargets targets;
	targets.targets.push_back({"linux", "linux", "", {{"channel", "release"}}});
	targets.targets.push_back({"web", "web", "../site", {}});
	std::string error;
	REQUIRE(targets.Save(fixture.project, error));

	ExportTargets loaded;
	REQUIRE(loaded.Load(fixture.project, error));
	REQUIRE(loaded.targets.size() == 2);
	CHECK(loaded.targets[0].name == "linux");
	CHECK(loaded.targets[0].params ==
		  std::vector<std::pair<std::string, std::string>>{{"channel", "release"}});
	CHECK(loaded.targets[1].output == "../site");
	CHECK(ExportTargetOutput(fixture.project, loaded.targets[0]) ==
		  fixture.project / "export" / "linux");
	CHECK(ExportTargetOutput(fixture.project, loaded.targets[1]) == fixture.root / "site");

	CHECK(ValidateExportTargetShape(loaded, loaded.targets[0]).empty());
	loaded.targets[1].name = "linux";
	CHECK(ValidateExportTargetShape(loaded, loaded.targets[1]).find("more than once") !=
		  std::string::npos);
	loaded.targets[1].name = "bad name";
	CHECK_FALSE(ValidateExportTargetShape(loaded, loaded.targets[1]).empty());
	loaded.targets[1].name = "ok";
	loaded.targets[1].platform = "ps5";
	CHECK(ValidateExportTargetShape(loaded, loaded.targets[1]).find("ps5") != std::string::npos);
}

TEST_CASE("building targets stages files, applies overrides and lays out each platform") {
	BuildFixture fixture;
	fixture.script(kScript);
	fixture.targets(kTargets);

	const BuildReport report = fixture.build({"linux", "demo", "web"});
	REQUIRE_MESSAGE(report.ok, report.error);
	REQUIRE(report.built.size() == 3);

	const fs::path linuxOut = fixture.project / "export" / "linux";
	CHECK(read(linuxOut / "My Game") == "linux engine");
	CHECK((fs::status(linuxOut / "My Game").permissions() & fs::perms::owner_exec) !=
		  fs::perms::none);
	CHECK(read(linuxOut / "LICENSE.txt") == "license");
	CHECK(read(linuxOut / "steam_api.dll") == "dll");
	CHECK(read(linuxOut / "licenses" / "lua" / "LICENSE") == "lua license");
	const fs::path linuxZip = linuxOut / "My Game.zip";
	CHECK(archived(linuxZip, "scenes/world2/boss.scene") == "boss");
	CHECK(archived(linuxZip, "secret/key.txt") == "<missing>");
	CHECK(archived(linuxZip, ".env") == "<missing>");
	CHECK(archived(linuxZip, ".data/save.txt") == "save");
	CHECK(archived(linuxZip, "build.lua") == "<missing>");
	CHECK(archived(linuxZip, "export_targets.sowa") == "<missing>");
	CHECK(archived(linuxZip, "vendor/steam/steam_api.dll") == "<missing>");
	CHECK(archived(linuxZip, "info.txt").rfind("linux linux 1.2.0\nfiles=", 0) == 0);
	const std::string linuxProject = archived(linuxZip, "project.sowa");
	CHECK(linuxProject.find("channel: release") != std::string::npos);
	CHECK(linuxProject.find("Width: 1280") != std::string::npos);
	CHECK(linuxProject.find("TicksPerSecond: 120") != std::string::npos);

	const fs::path demo = fixture.project / "export" / "demo";
	CHECK(read(demo / "My Game Demo.exe") == "windows engine");
	CHECK(archived(demo / "My Game Demo.zip", "scenes/world2/boss.scene") == "<missing>");
	CHECK(archived(demo / "My Game Demo.zip", "project.sowa").find("Name: My Game Demo") !=
		  std::string::npos);
	// Each target's archive leaves out the other targets' outputs.
	CHECK(archived(demo / "My Game Demo.zip", "export/linux/LICENSE.txt") == "<missing>");

	const fs::path site = fixture.root / "site";
	CHECK(read(site / "sowa.js") == "js");
	CHECK(read(site / "sowa.wasm") == "wasm");
	CHECK(read(site / "index.html").find("<title>My Game</title>") != std::string::npos);
	CHECK(archived(site / "game.zip", ".env") == "ENV=web");
	CHECK(archived(site / "game.zip", "project.sowa").find("Width: 960") != std::string::npos);

	CHECK(archived(site / "game.zip", "project.sowa").find("TicksPerSecond: 30") !=
		  std::string::npos);

	// One timestamp per run.
	CHECK(archived(linuxZip, "stamp.txt") == archived(site / "game.zip", "stamp.txt"));
	CHECK(fs::exists(fixture.project / ".sowa" / "exports" / "web.manifest"));
}

TEST_CASE("build contexts collect callbacks safely between targets") {
	BuildFixture fixture;
	fixture.script(R"(
build.platforms { Platform.Linux }
build.on("configure", function(ctx)
    collectgarbage("collect")
    ctx:mount("res"):include("scenes/**")
end)
)");
	fixture.targets(R"(Targets:
  - { Name: first, Platform: linux }
  - { Name: second, Platform: linux }
  - { Name: third, Platform: linux }
  - { Name: fourth, Platform: linux }
)");
	const BuildReport report = fixture.build({"first", "second", "third", "fourth"});
	REQUIRE_MESSAGE(report.ok, report.error);
	CHECK(report.built.size() == 4);
}

TEST_CASE("web exports use the project icon as the favicon") {
	BuildFixture fixture;
	write(fixture.project / "project.sowa",
		"Application:\n  Name: Icons\n  Icon: res://icon.png\n");
	fixture.targets("Targets:\n  - Name: web\n    Platform: web\n");
	REQUIRE_MESSAGE(fixture.build({"web"}).ok, "web build");
	const fs::path out = fixture.project / "export" / "web";
	CHECK(read(out / "favicon.png") == "png");
	CHECK(read(out / "index.html").find("<link rel=\"icon\" href=\"favicon.png\">") !=
		  std::string::npos);
}

TEST_CASE("rebuilds replace only their own files and refuse to overwrite others") {
	BuildFixture fixture;
	fixture.targets("Targets:\n  - Name: linux\n    Platform: linux\n");
	REQUIRE(fixture.build({"linux"}).ok);
	const fs::path out = fixture.project / "export" / "linux";

	write(out / "server.cfg", "mine");
	write(fixture.project / "scenes" / "new.scene", "new");
	REQUIRE(fixture.build({"linux"}).ok);
	CHECK(read(out / "server.cfg") == "mine");
	CHECK(archived(out / "My Game.zip", "scenes/new.scene") == "new");

	// Without the manifest nothing in the folder is provably ours.
	fs::remove(fixture.project / ".sowa" / "exports" / "linux.manifest");
	const BuildReport conflict = fixture.build({"linux"});
	CHECK_FALSE(conflict.ok);
	CHECK(conflict.error.find("did not create") != std::string::npos);
	CHECK(read(out / "server.cfg") == "mine");

	// Files the new build no longer produces are removed.
	fs::remove_all(out);
	fixture.script(R"(build.platforms { Platform.Linux }
build.on("configure", function(ctx)
	ctx:mount("res"):include("**")
	ctx:output_dir():include("LICENSE.txt")
end))");
	REQUIRE(fixture.build({"linux"}).ok);
	CHECK(fs::exists(out / "LICENSE.txt"));
	fixture.script(R"(build.platforms { Platform.Linux }
build.on("configure", function(ctx) ctx:mount("res"):include("**") end))");
	REQUIRE(fixture.build({"linux"}).ok);
	CHECK_FALSE(fs::exists(out / "LICENSE.txt"));
}

TEST_CASE("ctx:abort fails the build with its message and runs failed handlers") {
	BuildFixture fixture;
	fixture.script(std::string(kScript) + R"(
build.on("failed", function(ctx, message)
	log.info("failed handler: {}", message)
end)
)");
	fixture.targets(kTargets);

	const BuildReport report = fixture.build({"linux", "web"}, {{"server", "true"}});
	CHECK_FALSE(report.ok);
	CHECK(report.error == "Target 'web': Can not create a server build on web");
	REQUIRE(report.built.size() == 1);
	CHECK(report.built[0].first == "linux");
	CHECK_FALSE(fs::exists(fixture.root / "site"));
}

TEST_CASE("--param overrides are validated against build.params") {
	BuildFixture fixture;
	fixture.script(kScript);
	fixture.targets(kTargets);

	BuildReport report = fixture.build({"linux"}, {{"channel", "nightly"}});
	CHECK_FALSE(report.ok);
	CHECK(report.error.find("param 'channel'") != std::string::npos);
	report = fixture.build({"linux"}, {{"unknown", "1"}});
	CHECK_FALSE(report.ok);
	CHECK(report.error.find("unknown param 'unknown'") != std::string::npos);

	report = fixture.build({"linux"}, {{"channel", "beta"}});
	REQUIRE_MESSAGE(report.ok, report.error);
	CHECK(archived(fixture.project / "export" / "linux" / "My Game.zip", "project.sowa")
			  .find("channel: beta") != std::string::npos);
	// The saved target is untouched.
	CHECK(read(fixture.project / "export_targets.sowa") == kTargets);
}

TEST_CASE("build checks every target before building any") {
	BuildFixture fixture;
	fixture.script(kScript);
	fixture.targets(R"(
Targets:
  - Name: linux
    Platform: linux
    Params: { channel: release }
  - Name: mac
    Platform: macos
    Params: { channel: release }
  - Name: missing
    Platform: linux
)");
	BuildReport report = fixture.build({"linux", "mac"});
	CHECK_FALSE(report.ok);
	CHECK(report.error.find("does not support platform 'macos'") != std::string::npos);
	CHECK_FALSE(fs::exists(fixture.project / "export" / "linux"));

	report = fixture.build({"linux", "missing"});
	CHECK(report.error == "Target 'missing': param 'channel' is required.");
	CHECK_FALSE(fs::exists(fixture.project / "export" / "linux"));

	report = fixture.build({"nope"});
	CHECK(report.error.find("No target named 'nope'") != std::string::npos);

	fs::remove_all(fixture.templates / "linux");
	report = fixture.build({"linux"});
	CHECK(report.error.find("export template") != std::string::npos);
}

TEST_CASE("build.lua mistakes report the script location") {
	BuildFixture fixture;
	fixture.targets("Targets:\n  - Name: linux\n    Platform: linux\n");
	const auto failure = [&](std::string_view body) {
		fixture.script("build.platforms { Platform.Linux }\nbuild.on(\"configure\", "
					   "function(ctx)\n" +
					   std::string(body) + "\nend)\n");
		const BuildReport report = fixture.build({"linux"});
		CHECK_FALSE(report.ok);
		return report.error;
	};

	CHECK(failure("ctx.project.aplication.name = 'x'") ==
		  "Target 'linux': build.lua:3: unknown project setting 'ctx.project.aplication'");
	CHECK(failure("ctx.project.rendering.window.width = 'wide'") ==
		  "Target 'linux': build.lua:3: ctx.project.rendering.window.width must be an "
		  "integer, got string");
	CHECK(failure("ctx.project.physics.ticks_per_second = 'fast'") ==
		  "Target 'linux': build.lua:3: ctx.project.physics.ticks_per_second must be an "
		  "integer, got string");
	CHECK(failure("ctx.project.physics.ticks_per_second = 0").find(
			  "ctx.project.physics.ticks_per_second must be between 1 and") != std::string::npos);
	CHECK(failure("ctx.project.global_store.lives = 3") ==
		  "Target 'linux': build.lua:3: ctx.project.global_store.lives must be a string, got "
		  "integer");
	CHECK(failure("ctx:mount('res'):include('scenes')") ==
		  "Target 'linux': ctx:mount(\"res\"): 'scenes' is a directory; use 'scenes/**'");
	CHECK(failure("ctx:mount('res'):include('nope.txt')") ==
		  "Target 'linux': ctx:mount(\"res\"): 'nope.txt' does not exist");
	CHECK(failure("ctx:mount('res'):include('build.lua')") ==
		  "Target 'linux': ctx:mount(\"res\"): 'build.lua' is never exported");
	CHECK(failure("ctx:mount('res'):include('**', { as = 'x' })") ==
		  "Target 'linux': build.lua:3: include: as needs a single file path, not a pattern");
	CHECK(failure("ctx:mount('assets')") ==
		  "Target 'linux': build.lua:3: ctx:mount: only \"res\" can be mounted");
	CHECK(failure("ctx.params.x = 1") == "Target 'linux': build.lua:3: ctx.params is read-only");
	CHECK(failure("ctx:output_dir():include('LICENSE.txt'):write('My Game.zip', 'x')") ==
		  "Target 'linux': ctx:output_dir(): 'My Game.zip' is a file the export itself writes");

	fixture.script(R"(build.platforms { Platform.Linux }
build.on("configure", function(ctx) ctx:mount("res"):include("**") end)
build.on("staged", function(ctx) ctx:mount("res"):exclude("**") end))");
	BuildReport report = fixture.build({"linux"});
	CHECK(report.error ==
		  "Target 'linux': build.lua:3: files can only be added or removed in configure");

	fixture.script(R"(build.platforms { Platform.Linux }
build.on("configure", function(ctx) ctx:mount("res"):include("**") end)
build.on("staged", function(ctx) ctx.project.application.name = "late" end))");
	report = fixture.build({"linux"});
	CHECK(report.error == "Target 'linux': build.lua:3: ctx.project.application.name can only "
						  "be changed in configure");
}

TEST_CASE("build.lua declarations are validated when it loads") {
	BuildFixture fixture;
	fixture.targets("Targets:\n  - Name: linux\n    Platform: linux\n");
	const auto loadError = [&](std::string_view text) {
		fixture.script(text);
		return fixture.build({"linux"}).error;
	};
	CHECK(loadError("build.params { x = Param.Int { default = 1 } }") ==
		  "build.lua must call build.platforms { ... }");
	CHECK(loadError("build.platforms { 'ps5' }").find("entry 1 is not a Platform value") !=
		  std::string::npos);
	CHECK(loadError("build.platforms { Platform.PS5 }") ==
		  "build.lua:1: Platform has no field 'PS5'");
	CHECK(loadError("build.platforms { Platform.Linux } build.params { x = Param.Bool { "
					"defualt = true } }") ==
		  "build.lua:1: param 'x' (Param.Bool): unknown field 'defualt'");
	CHECK(loadError("build.platforms { Platform.Linux } build.params { x = Param.Int { "
					"default = 1, required = true } }") ==
		  "build.lua:1: param 'x' (Param.Int): a required param cannot have a default");
	CHECK(loadError("build.platforms { Platform.Linux } build.params { x = Param.Int { "
					"default = 9, max = 4 } }") ==
		  "build.lua:1: param 'x' (Param.Int): default 9 is above the maximum 4");
	CHECK(loadError("build.platforms { Platform.Linux } build.on('finish', function() end)") ==
		  "build.lua:1: build.on: unknown event; expected configure, staged, done or failed");
}

TEST_CASE("params keep declaration order and require loads project modules") {
	BuildFixture fixture;
	write(fixture.project / "build" / "names.lua", "return { suffix = '!' }");
	fixture.script(R"(
local names = require("build/names")
build.params {
    zeta  = Param.String { default = "z" },
    alpha = Param.Int { default = 1 },
    mid   = Param.Float { default = 0.5 },
}
build.platforms { Platform.Linux }
build.on("configure", function(ctx)
    ctx:mount("res"):write("x.txt", ctx.params.zeta .. names.suffix)
end)
)");
	fixture.targets("Targets:\n  - Name: linux\n    Platform: linux\n");

	BuildScript script;
	std::string error;
	REQUIRE_MESSAGE(script.Load(fixture.project, error), error);
	REQUIRE(script.Params().size() == 3);
	CHECK(script.Params()[0].name == "zeta");
	CHECK(script.Params()[1].name == "alpha");
	CHECK(script.Params()[2].name == "mid");

	const BuildReport report = fixture.build({"linux"});
	REQUIRE_MESSAGE(report.ok, report.error);
	const fs::path zip = fixture.project / "export" / "linux" / "My Game.zip";
	CHECK(archived(zip, "x.txt") == "z!");
	// The res builder only has the written file; project.sowa is always added.
	CHECK(archived(zip, "scenes/main.scene") == "<missing>");
	CHECK(archived(zip, "project.sowa").find("My Game") != std::string::npos);
}

TEST_CASE("projects without build.lua export everything for any platform") {
	BuildFixture fixture;
	fixture.targets("Targets:\n  - Name: mac\n    Platform: macos\n");
	BuildScript script;
	std::string error;
	REQUIRE(script.Load(fixture.project, error));
	CHECK_FALSE(script.HasBuildFile());
	CHECK(script.Platforms().size() == 4);

	const BuildReport report = fixture.build({"mac"});
	REQUIRE_MESSAGE(report.ok, report.error);
	const fs::path app = fixture.project / "export" / "mac" / "My Game.app" / "Contents";
	CHECK(read(app / "MacOS" / "My Game") == "mac engine");
	const std::string plist = read(app / "Info.plist");
	CHECK(plist.find("<key>CFBundleExecutable</key>\n\t<string>My Game</string>") !=
		  std::string::npos);
	CHECK(plist.find("<string>com.sowa.my-game</string>") != std::string::npos);
	// No icon configured: no icon file, no CFBundleIconFile.
	CHECK(plist.find("CFBundleIconFile") == std::string::npos);
	CHECK_FALSE(fs::exists(app / "Resources" / "AppIcon.icns"));
	const fs::path zip = app / "Resources" / "My Game.zip";
	CHECK(archived(zip, "scenes/main.scene") == "scene");
	CHECK(archived(zip, "secret/key.txt") == "key");
	CHECK(archived(zip, ".env") == "<missing>");
}

TEST_CASE("symlinks leaving the project are rejected") {
	BuildFixture fixture;
	write(fixture.root / "outside" / "file.txt", "outside");
	std::error_code ec;
	fs::create_directory_symlink(fixture.root / "outside", fixture.project / "linked", ec);
	if (ec) {
		return; // Symlinks need extra privileges on some Windows setups.
	}
	fixture.targets("Targets:\n  - Name: linux\n    Platform: linux\n");
	const BuildReport report = fixture.build({"linux"});
	CHECK_FALSE(report.ok);
	CHECK(report.error.find("'linked' links outside the project") != std::string::npos);
}

TEST_CASE("macOS bundles get an AppIcon.icns from the project icon") {
	BuildFixture fixture;
	std::vector<unsigned char> pixels(8 * 8 * 4, 255);
	std::size_t length = 0;
	void *png =
		tdefl_write_image_to_png_file_in_memory_ex(pixels.data(), 8, 8, 4, &length, 6, false);
	write(fixture.project / "icon.png", std::string(static_cast<const char *>(png), length));
	mz_free(png);
	write(fixture.project / "project.sowa",
		"Application:\n  Name: Icons\n  Icon: res://icon.png\n");
	fixture.targets("Targets:\n  - Name: mac\n    Platform: macos\n");

	const BuildReport report = fixture.build({"mac"});
	REQUIRE_MESSAGE(report.ok, report.error);
	const fs::path contents = fixture.project / "export" / "mac" / "Icons.app" / "Contents";
	CHECK(read(contents / "Resources" / "AppIcon.icns").substr(0, 4) == "icns");
	CHECK(
		read(contents / "Info.plist")
			.find("<key>CFBundleIconFile</key>\n\t<string>AppIcon</string>") != std::string::npos);

	// An undecodable icon is a warning, not a failure.
	write(fixture.project / "icon.png", "not a png");
	REQUIRE(fixture.build({"mac"}).ok);
	CHECK_FALSE(fs::exists(contents / "Resources" / "AppIcon.icns"));
}
