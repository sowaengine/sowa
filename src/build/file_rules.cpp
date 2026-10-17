#include "build/file_rules.hpp"

#include <algorithm>
#include <map>
#include <system_error>

namespace sowa::build {
namespace {

bool isExcluded(const ResolveContext &context, const std::string &path) {
	for (const std::string &excluded : context.excluded) {
		if (path == excluded ||
			(path.size() > excluded.size() && path.compare(0, excluded.size(), excluded) == 0 &&
				path[excluded.size()] == '/')) {
			return true;
		}
	}
	return false;
}

bool insideRoot(const std::filesystem::path &root, const std::filesystem::path &path) {
	std::error_code ec;
	const auto canonical = std::filesystem::canonical(path, ec);
	if (ec) {
		return false;
	}
	const auto relative = canonical.lexically_relative(root);
	return !relative.empty() && *relative.begin() != "..";
}

std::string join(const std::string &a, const std::string &b) {
	if (a.empty() || a == ".") {
		return b;
	}
	return a + "/" + b;
}

class Resolver {
  public:
	Resolver(const ResolveContext &context, std::string &error) : _context(context), _error(error) {
	}

	bool Include(const FileRule &rule, std::vector<std::string> &warnings) {
		const GlobPattern &pattern = *rule.pattern;
		std::vector<std::string> matches;
		if (pattern.IsExplicit()) {
			if (!explicitFile(pattern.Text())) {
				return false;
			}
			matches.push_back(pattern.Text());
		} else if (!walk(pattern, pattern.FixedPrefix(), matches)) {
			return false;
		}
		if (matches.empty()) {
			warnings.push_back("'" + pattern.Text() + "' matched no files");
			return true;
		}

		const std::string prefix = pattern.FixedPrefix();
		for (const std::string &source : matches) {
			std::string destination = source;
			if (rule.as) {
				destination = *rule.as;
			} else if (rule.into) {
				const std::string below =
					prefix.empty() ? source : source.substr(prefix.size() + 1);
				destination = join(*rule.into, below);
			}
			ResolvedFile file;
			file.destination = destination;
			file.source = ResolvedFile::Source::File;
			file.file = _context.projectRoot / source;
			_files[destination] = std::move(file);
		}
		return true;
	}

	void Exclude(const GlobPattern &pattern) {
		for (auto it = _files.begin(); it != _files.end();) {
			it = pattern.Matches(it->first) ? _files.erase(it) : std::next(it);
		}
	}

	void Add(ResolvedFile file) {
		std::string destination = file.destination;
		_files[destination] = std::move(file);
	}

	std::vector<ResolvedFile> Take() {
		std::vector<ResolvedFile> files;
		files.reserve(_files.size());
		for (auto &[destination, file] : _files) {
			(void)destination;
			files.push_back(std::move(file));
		}
		return files;
	}

  private:
	bool explicitFile(const std::string &path) {
		if (isExcluded(_context, path)) {
			_error = "'" + path + "' is never exported";
			return false;
		}
		const auto absolute = _context.projectRoot / path;
		std::error_code ec;
		const auto status = std::filesystem::status(absolute, ec);
		if (!std::filesystem::exists(status)) {
			_error = "'" + path + "' does not exist";
			return false;
		}
		if (!insideRoot(_context.projectRoot, absolute)) {
			_error = "'" + path + "' resolves outside the project";
			return false;
		}
		if (std::filesystem::is_directory(status)) {
			_error = "'" + path + "' is a directory; use '" + path + "/**'";
			return false;
		}
		return true;
	}

	bool walk(const GlobPattern &pattern, const std::string &directory,
		std::vector<std::string> &matches) {
		const auto absolute = _context.projectRoot / directory;
		std::error_code ec;
		if (!std::filesystem::is_directory(absolute, ec)) {
			return true;
		}
		std::vector<std::filesystem::directory_entry> entries;
		for (const auto &entry : std::filesystem::directory_iterator(absolute, ec)) {
			entries.push_back(entry);
		}
		if (ec) {
			_error = "cannot read '" + (directory.empty() ? "." : directory) + "': " + ec.message();
			return false;
		}
		std::sort(entries.begin(), entries.end(),
			[](const auto &a, const auto &b) { return a.path() < b.path(); });

		for (const auto &entry : entries) {
			const std::string path = join(directory, entry.path().filename().generic_string());
			if (isExcluded(_context, path)) {
				continue;
			}
			const bool directoryEntry = entry.is_directory(ec);
			const bool candidate =
				directoryEntry ? pattern.CouldMatchBelow(path) : pattern.Matches(path);
			if (!candidate) {
				continue;
			}
			if (entry.is_symlink(ec) && !insideRoot(_context.projectRoot, entry.path())) {
				_error = "'" + path + "' links outside the project";
				return false;
			}
			if (directoryEntry) {
				if (!walk(pattern, path, matches)) {
					return false;
				}
			} else if (entry.is_regular_file(ec)) {
				matches.push_back(path);
			}
		}
		return true;
	}

	const ResolveContext &_context;
	std::string &_error;
	std::map<std::string, ResolvedFile> _files;
};

} // namespace

std::optional<std::vector<ResolvedFile>> ResolveRules(const std::vector<FileRule> &rules,
	const ResolveContext &context, std::vector<std::string> &warnings, std::string &error) {
	Resolver resolver(context, error);
	for (const FileRule &rule : rules) {
		switch (rule.kind) {
		case FileRule::Kind::Include:
			if (!resolver.Include(rule, warnings)) {
				return std::nullopt;
			}
			break;
		case FileRule::Kind::Exclude:
			resolver.Exclude(*rule.pattern);
			break;
		case FileRule::Kind::Write: {
			ResolvedFile file;
			file.destination = rule.path;
			file.source = ResolvedFile::Source::Content;
			file.content = rule.content;
			resolver.Add(std::move(file));
			break;
		}
		case FileRule::Kind::WriteFunction: {
			ResolvedFile file;
			file.destination = rule.path;
			file.source = ResolvedFile::Source::Function;
			file.function = rule.function;
			resolver.Add(std::move(file));
			break;
		}
		}
	}
	auto files = resolver.Take();
	// A file and a directory cannot share a path ("a" and "a/b").
	for (std::size_t i = 0; i + 1 < files.size(); ++i) {
		const std::string &a = files[i].destination;
		for (std::size_t j = i + 1; j < files.size(); ++j) {
			const std::string &b = files[j].destination;
			if (b.compare(0, a.size(), a) != 0) {
				break;
			}
			if (b.size() > a.size() && b[a.size()] == '/') {
				error = "'" + a + "' is both a file and a directory in the output";
				return std::nullopt;
			}
		}
	}
	return files;
}

} // namespace sowa::build
