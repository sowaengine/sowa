#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace sowa::build {

// Validates and normalizes a relative build path: forward slashes only, no
// absolute paths, no empty, "." or ".." segments; a leading "./" is
// dropped. Returns the normalized path, or nullopt with error set.
std::optional<std::string> NormalizeBuildPath(std::string_view path, std::string &error);

// Project-relative glob: * matches within a segment, ** matches whole segments,
// and ? matches one character. Dotfiles require an explicitly dot-prefixed segment.
class GlobPattern {
  public:
	static std::optional<GlobPattern> Parse(std::string_view pattern, std::string &error);

	// True for a pattern without wildcards: it names exactly one path.
	bool IsExplicit() const {
		return _explicit;
	}

	const std::string &Text() const {
		return _text;
	}

	// The leading segments without wildcards, joined with '/'. For an explicit
	// pattern, its parent directory. Empty when the first segment is a wildcard.
	std::string FixedPrefix() const;

	bool Matches(std::string_view path) const;

	// Whether some path inside directory path could match, so a walk can
	// skip directories that cannot contain a match.
	bool CouldMatchBelow(std::string_view path) const;

  private:
	std::string _text;
	std::vector<std::string> _segments;
	bool _explicit = true;
};

std::vector<std::string> SplitPath(std::string_view path);

} // namespace sowa::build
