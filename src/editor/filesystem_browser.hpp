#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "core/filesystem/fs_types.hpp"

namespace sowa {

class FileSystem;
class SceneManager;

namespace editor {

// What a browser row is, from its name (and whether it is a directory).
// Used to pick icons and the double-click / context action.
enum class AssetKind {
	Directory,
	Scene,
	Script,
	Image,
	Audio,
	Font,
	Other,
};

enum class AssetAction {
	None,
	OpenScene,
};

enum class AssetCreateKind {
	File,
	Directory,
	Scene,
	Script,
};

AssetKind KindFromName(std::string_view name, bool isDirectory);
AssetAction PrimaryAction(AssetKind kind);

// res:// + child, or res://dir + child. Does not insert a slash after
// "://", so the project root stays res://file rather than res:///file.
std::string JoinVirtualPath(std::string_view parent, std::string_view name);

// Validate leaf names to prevent directory escapes, hidden files, and platform-reserved names.
bool IsValidAssetName(std::string_view name);
std::string AssetName(std::string_view path);
std::string ParentVirtualPath(std::string_view path);

// Filesystem CRUD with resulting paths for selection. Never overwrite existing files
// or rename/delete the mount root. Scene and script templates roll back failed writes.
FsResult<std::string> CreateAsset(FileSystem &fs, std::string_view parent, std::string_view name,
	AssetCreateKind kind);
FsResult<std::string> RenameAsset(FileSystem &fs, std::string_view path, std::string_view newName);
FsError DeleteAsset(FileSystem &fs, std::string_view path);

// Directory listing for the panel: skips hidden names (leading '.'), then
// directories first, then files, both by case-insensitive name.
std::vector<FileEntry> ListAssets(FileSystem &fs, std::string_view dirPath);

// Focus an already-loaded scene, or open and focus a new scene document.
// Focusing enables that document and pauses the other tabs. False while
// playing, if the kind has no Open action, or if the file fails to load.
bool TryOpenAsset(SceneManager &scenes, std::string_view path, AssetKind kind);

} // namespace editor
} // namespace sowa
