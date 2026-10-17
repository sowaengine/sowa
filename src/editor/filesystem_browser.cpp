#include "editor/filesystem_browser.hpp"

#include <algorithm>
#include <cctype>
#include <cstring>

#include "core/filesystem/file_system.hpp"
#include "core/serialize/archive.hpp"
#include "core/string.hpp"
#include "editor/scene_documents.hpp"
#include "scene/scene.hpp"
#include "scene/scene_manager.hpp"

namespace sowa::editor {
namespace {

std::string_view extensionOf(std::string_view name) {
	const std::size_t slash = name.find_last_of('/');
	const std::string_view file = slash == std::string_view::npos ? name : name.substr(slash + 1);
	const std::size_t dot = file.find_last_of('.');
	if (dot == std::string_view::npos || dot == 0) {
		return {};
	}
	return file.substr(dot);
}

bool hiddenName(std::string_view name) {
	return name.empty() || name[0] == '.';
}

bool isVirtualRoot(FileSystem &fs, std::string_view path) {
	const auto resolved = fs.Resolve(path);
	return resolved && resolved.value.path.empty();
}

} // namespace

AssetKind KindFromName(std::string_view name, bool isDirectory) {
	if (isDirectory) {
		return AssetKind::Directory;
	}
	const std::string ext = String::ToLower(extensionOf(name));
	if (ext == ".scene") {
		return AssetKind::Scene;
	}
	if (ext == ".lua") {
		return AssetKind::Script;
	}
	if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".tga" || ext == ".bmp" ||
		ext == ".webp") {
		return AssetKind::Image;
	}
	if (ext == ".wav" || ext == ".ogg" || ext == ".mp3") {
		return AssetKind::Audio;
	}
	if (ext == ".ttf" || ext == ".otf") {
		return AssetKind::Font;
	}
	return AssetKind::Other;
}

AssetAction PrimaryAction(AssetKind kind) {
	if (kind == AssetKind::Scene) {
		return AssetAction::OpenScene;
	}
	return AssetAction::None;
}

std::string JoinVirtualPath(std::string_view parent, std::string_view name) {
	if (parent.empty()) {
		return std::string(name);
	}
	if (name.empty()) {
		return std::string(parent);
	}
	if (parent.size() >= 3 && parent.substr(parent.size() - 3) == "://") {
		return std::string(parent) + std::string(name);
	}
	if (parent.back() == '/') {
		return std::string(parent) + std::string(name);
	}
	return std::string(parent) + "/" + std::string(name);
}

bool IsValidAssetName(std::string_view name) {
	if (name.empty() || name == "." || name == ".." || name.front() == '.' ||
		std::isspace(static_cast<unsigned char>(name.front())) ||
		std::isspace(static_cast<unsigned char>(name.back())) || name.back() == '.') {
		return false;
	}

	for (const unsigned char character : name) {
		if (std::iscntrl(character) ||
			std::string_view("<>:\"/\\|?*").find(character) != std::string_view::npos) {
			return false;
		}
	}
	return true;
}

std::string AssetName(std::string_view path) {
	const size_t scheme = path.find("://");
	const size_t rootEnd = scheme == std::string_view::npos ? 0 : scheme + 3;
	while (path.size() > rootEnd && path.back() == '/') {
		path.remove_suffix(1);
	}
	if (path.size() <= rootEnd) {
		return {};
	}
	const size_t slash = path.find_last_of('/');
	return std::string(slash == std::string_view::npos ? path : path.substr(slash + 1));
}

std::string ParentVirtualPath(std::string_view path) {
	const size_t scheme = path.find("://");
	if (scheme == std::string_view::npos) {
		return {};
	}
	const size_t rootEnd = scheme + 3;
	while (path.size() > rootEnd && path.back() == '/') {
		path.remove_suffix(1);
	}
	const size_t slash = path.find_last_of('/');
	if (slash == std::string_view::npos || slash < rootEnd) {
		return std::string(path.substr(0, rootEnd));
	}
	return std::string(path.substr(0, slash));
}

FsResult<std::string> CreateAsset(FileSystem &fs, std::string_view parent, std::string_view name,
	AssetCreateKind kind) {
	if (!IsValidAssetName(name) || !fs.Resolve(parent)) {
		return {{}, FsError::InvalidPath};
	}
	std::string filename(name);
	const bool scene = kind == AssetCreateKind::Scene;
	const bool script = kind == AssetCreateKind::Script;
	if (scene || script) {
		const std::string_view extension = scene ? ".scene" : ".lua";
		const auto supplied = extensionOf(filename);
		if (supplied.empty()) {
			filename += extension;
		} else if (String::ToLower(supplied) != extension) {
			return {{}, FsError::InvalidPath};
		}
	}

	std::string contents;
	if (scene || script) {
		std::string stem = filename.substr(0, filename.find_last_of('.'));
		if (scene) {
			auto archive = Archive::Save();
			auto root = archive.Child("root");
			root.Field("name", stem);
			auto components = root.Child("components");
			components.Child("SceneRoot");
			components.Child("Transform2D");
			contents = archive.ToString();
		} else {
			// A safe component identifier even when the filename contains spaces.
			for (char &c : stem) {
				if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
						c == '_')) {
					c = '_';
				}
			}
			if (stem.front() >= '0' && stem.front() <= '9') {
				stem = "Script_" + stem;
			}
			contents = "local Component = world:component(\"" + stem +
					   "\", {})\n\n"
					   "function Component:_start()\n"
					   "\t-- Called when this entity enters the scene.\nend\n\n"
					   "function Component:_update(dt)\n"
					   "\t-- Update this entity each frame.\nend\n\n"
					   "return Component\n";
		}
	}

	const std::string path = JoinVirtualPath(parent, filename);
	FsError error =
		kind == AssetCreateKind::Directory ? fs.CreateDirectory(path) : fs.CreateFile(path);
	if (error == FsError::Ok && !contents.empty()) {
		FileData data;
		data.bytes.resize(contents.size());
		std::memcpy(data.Data(), contents.data(), contents.size());
		error = fs.Save(path, data);
		if (error != FsError::Ok) {
			fs.RemoveAll(path);
		}
	}
	return {error == FsError::Ok ? path : std::string(), error};
}

FsResult<std::string> RenameAsset(FileSystem &fs, std::string_view path, std::string_view newName) {
	if (!IsValidAssetName(newName) || isVirtualRoot(fs, path)) {
		return {{}, FsError::InvalidPath};
	}
	const auto resolved = fs.Resolve(path);
	if (!resolved) {
		return {{}, resolved.error};
	}
	const std::string parent = ParentVirtualPath(path);
	if (parent.empty()) {
		return {{}, FsError::InvalidPath};
	}
	const std::string renamed = JoinVirtualPath(parent, newName);
	if (renamed == path) {
		return {renamed, FsError::Ok};
	}
	const FsError error = fs.Move(path, renamed);
	return {error == FsError::Ok ? renamed : std::string(), error};
}

FsError DeleteAsset(FileSystem &fs, std::string_view path) {
	const auto resolved = fs.Resolve(path);
	if (!resolved) {
		return resolved.error;
	}
	if (resolved.value.path.empty()) {
		return FsError::InvalidPath;
	}
	return fs.RemoveAll(path);
}

std::vector<FileEntry> ListAssets(FileSystem &fs, std::string_view dirPath) {
	auto listing = fs.ReadDirectory(dirPath);
	std::vector<FileEntry> out;
	if (!listing) {
		return out;
	}
	out.reserve(listing.value.size());
	for (FileEntry &entry : listing.value) {
		if (hiddenName(entry.name)) {
			continue;
		}
		out.push_back(std::move(entry));
	}
	std::sort(out.begin(), out.end(), [](const FileEntry &a, const FileEntry &b) {
		if (a.isDirectory != b.isDirectory) {
			return a.isDirectory && !b.isDirectory;
		}
		return String::ToLower(a.name) < String::ToLower(b.name);
	});
	return out;
}

bool TryOpenAsset(SceneManager &scenes, std::string_view path, AssetKind kind) {
	if (PrimaryAction(kind) != AssetAction::OpenScene) {
		return false;
	}
	if (scenes.IsPlaying()) {
		return false;
	}
	if (Scene existing = scenes.FindByPath(path); existing.IsValid()) {
		return ActivateSceneDocument(scenes, existing);
	}
	scenes.Open(path);
	scenes.Flush();
	Scene loaded = scenes.FindByPath(path);
	if (!loaded.IsValid()) {
		return false;
	}
	return ActivateSceneDocument(scenes, loaded);
}

} // namespace sowa::editor
