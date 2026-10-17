#include "build/glob.hpp"

namespace sowa::build {
namespace {

bool hasWildcard(std::string_view segment) {
	return segment.find_first_of("*?") != std::string_view::npos;
}

// Matches one path segment against one pattern segment with * and ?.
bool matchSegment(std::string_view pattern, std::string_view text) {
	if (!text.empty() && text.front() == '.' && (pattern.empty() || pattern.front() != '.')) {
		return false;
	}
	std::size_t p = 0;
	std::size_t t = 0;
	std::size_t starPattern = std::string_view::npos;
	std::size_t starText = 0;
	while (t < text.size()) {
		if (p < pattern.size() && (pattern[p] == '?' || pattern[p] == text[t])) {
			++p;
			++t;
		} else if (p < pattern.size() && pattern[p] == '*') {
			starPattern = p++;
			starText = t;
		} else if (starPattern != std::string_view::npos) {
			p = starPattern + 1;
			t = ++starText;
		} else {
			return false;
		}
	}
	while (p < pattern.size() && pattern[p] == '*') {
		++p;
	}
	return p == pattern.size();
}

bool isHidden(const std::string &segment) {
	return !segment.empty() && segment.front() == '.';
}

// prefixOnly: the path is a directory and the question is whether anything
// below it could match, i.e. whether the path is a viable prefix.
bool matchSegments(const std::vector<std::string> &pattern, std::size_t pi,
	const std::vector<std::string> &path, std::size_t si, bool prefixOnly) {
	if (si == path.size()) {
		if (prefixOnly) {
			return pi < pattern.size();
		}
		for (; pi < pattern.size(); ++pi) {
			if (pattern[pi] != "**") {
				return false;
			}
		}
		return true;
	}
	if (pi == pattern.size()) {
		return false;
	}
	if (pattern[pi] == "**") {
		if (matchSegments(pattern, pi + 1, path, si, prefixOnly)) {
			return true;
		}
		return !isHidden(path[si]) && matchSegments(pattern, pi, path, si + 1, prefixOnly);
	}
	return matchSegment(pattern[pi], path[si]) &&
		   matchSegments(pattern, pi + 1, path, si + 1, prefixOnly);
}

} // namespace

std::vector<std::string> SplitPath(std::string_view path) {
	std::vector<std::string> segments;
	std::size_t start = 0;
	while (start <= path.size()) {
		const std::size_t slash = path.find('/', start);
		const std::size_t end = slash == std::string_view::npos ? path.size() : slash;
		segments.emplace_back(path.substr(start, end - start));
		if (slash == std::string_view::npos) {
			break;
		}
		start = slash + 1;
	}
	return segments;
}

std::optional<std::string> NormalizeBuildPath(std::string_view path, std::string &error) {
	while (path.size() >= 2 && path.substr(0, 2) == "./") {
		path.remove_prefix(2);
	}
	if (path.empty()) {
		error = "path is empty";
		return std::nullopt;
	}
	if (path.find('\\') != std::string_view::npos) {
		error = "'" + std::string(path) + "' uses '\\'; use '/' as the separator";
		return std::nullopt;
	}
	if (path.front() == '/' || (path.size() >= 2 && path[1] == ':')) {
		error = "'" + std::string(path) + "' is absolute; paths are relative to the project";
		return std::nullopt;
	}
	for (const std::string &segment : SplitPath(path)) {
		if (segment.empty() || segment == ".") {
			error = "'" + std::string(path) + "' has an empty or '.' segment";
			return std::nullopt;
		}
		if (segment == "..") {
			error = "'" + std::string(path) + "' uses '..'; paths must stay inside the project";
			return std::nullopt;
		}
	}
	return std::string(path);
}

std::optional<GlobPattern> GlobPattern::Parse(std::string_view pattern, std::string &error) {
	const auto normalized = NormalizeBuildPath(pattern, error);
	if (!normalized) {
		return std::nullopt;
	}
	GlobPattern glob;
	glob._text = *normalized;
	glob._segments = SplitPath(glob._text);
	for (const std::string &segment : glob._segments) {
		if (segment.find("**") != std::string::npos && segment != "**") {
			error = "'" + glob._text + "': '**' must be a whole path segment";
			return std::nullopt;
		}
		if (hasWildcard(segment)) {
			glob._explicit = false;
		}
	}
	return glob;
}

std::string GlobPattern::FixedPrefix() const {
	std::string prefix;
	const std::size_t count = _explicit ? _segments.size() - 1 : _segments.size();
	for (std::size_t i = 0; i < count && !hasWildcard(_segments[i]); ++i) {
		if (!prefix.empty()) {
			prefix += '/';
		}
		prefix += _segments[i];
	}
	return prefix;
}

bool GlobPattern::Matches(std::string_view path) const {
	return matchSegments(_segments, 0, SplitPath(path), 0, false);
}

bool GlobPattern::CouldMatchBelow(std::string_view path) const {
	if (path.empty()) {
		return true;
	}
	return matchSegments(_segments, 0, SplitPath(path), 0, true);
}

} // namespace sowa::build
