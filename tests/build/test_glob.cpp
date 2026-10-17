#include <doctest/doctest.h>

#include "build/glob.hpp"

using namespace sowa::build;

namespace {

GlobPattern glob(const char *text) {
	std::string error;
	auto pattern = GlobPattern::Parse(text, error);
	REQUIRE_MESSAGE(pattern, error);
	return *pattern;
}

} // namespace

TEST_CASE("build paths are normalized relative paths") {
	std::string error;
	CHECK(NormalizeBuildPath("./scenes/a.scene", error) == "scenes/a.scene");
	CHECK_FALSE(NormalizeBuildPath("", error));
	CHECK_FALSE(NormalizeBuildPath("/etc/passwd", error));
	CHECK_FALSE(NormalizeBuildPath("C:/game", error));
	CHECK_FALSE(NormalizeBuildPath("a/../b", error));
	CHECK(error.find("..") != std::string::npos);
	CHECK_FALSE(NormalizeBuildPath("a//b", error));
	CHECK_FALSE(NormalizeBuildPath("a\\b", error));
}

TEST_CASE("globs match segments, ** spans directories") {
	CHECK(glob("**").Matches("project.sowa"));
	CHECK(glob("**").Matches("scenes/levels/one.scene"));
	CHECK(glob("scenes/*.scene").Matches("scenes/main.scene"));
	CHECK_FALSE(glob("scenes/*.scene").Matches("scenes/levels/one.scene"));
	CHECK(glob("scenes/**/*.scene").Matches("scenes/main.scene"));
	CHECK(glob("scenes/**/*.scene").Matches("scenes/levels/one.scene"));
	CHECK(glob("img?.png").Matches("img1.png"));
	CHECK_FALSE(glob("img?.png").Matches("img10.png"));
	CHECK(glob("**/*.md").Matches("docs/README.md"));
	CHECK(glob("secret/**").Matches("secret/key.txt"));
	CHECK_FALSE(glob("secret/**").Matches("secrets/key.txt"));
}

TEST_CASE("wildcards skip dotfiles unless the pattern names them") {
	CHECK_FALSE(glob("**").Matches(".env"));
	CHECK_FALSE(glob("**").Matches(".data/x.txt"));
	CHECK_FALSE(glob("*").Matches(".env"));
	CHECK_FALSE(glob("assets/*").Matches("assets/.DS_Store"));
	CHECK(glob(".data/**").Matches(".data/x.txt"));
	CHECK(glob("**/.DS_Store").Matches("assets/.DS_Store"));
	CHECK(glob(".env").Matches(".env"));
	CHECK(glob(".*").Matches(".env"));
}

TEST_CASE("glob prefixes and directory pruning") {
	CHECK(glob("vendor/steam/win64/*.dll").FixedPrefix() == "vendor/steam/win64");
	CHECK(glob("thirdparty/licenses/**").FixedPrefix() == "thirdparty/licenses");
	CHECK(glob("docs/README.md").FixedPrefix() == "docs");
	CHECK(glob("LICENSE.txt").FixedPrefix().empty());
	CHECK(glob("**/*.png").FixedPrefix().empty());
	CHECK(glob("docs/README.md").IsExplicit());
	CHECK_FALSE(glob("docs/*.md").IsExplicit());

	CHECK(glob("assets/**").CouldMatchBelow("assets"));
	CHECK(glob("assets/**").CouldMatchBelow("assets/music"));
	CHECK_FALSE(glob("assets/**").CouldMatchBelow("scenes"));
	CHECK_FALSE(glob("**").CouldMatchBelow(".git"));
	CHECK(glob(".data/**").CouldMatchBelow(".data"));
	CHECK(glob("**/*.png").CouldMatchBelow("a/b"));
}

TEST_CASE("** must be a whole segment") {
	std::string error;
	CHECK_FALSE(GlobPattern::Parse("a**/b", error));
	CHECK_FALSE(GlobPattern::Parse("../**", error));
}
