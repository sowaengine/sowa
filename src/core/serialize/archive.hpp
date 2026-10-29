#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <yaml-cpp/yaml.h>

#include "core/filesystem/fs_types.hpp"
#include "core/reflection/enum_constant.hpp"
#include "core/types.hpp"
#include "math/color.hpp"
#include "math/vector2.hpp"
#include "math/vector3.hpp"

namespace sowa {

class FileSystem;

using ResourceIdMapper = u64 (*)(u64 id, void *context);

// Serialize uses the same archive interface for saving and loading.
enum class ArchiveMode {
	Save,
	Load,
};

// Archive format version. Unversioned files read as version 0.
inline constexpr u32 kArchiveVersion = 1;

// Bidirectional YAML archive. Missing fields preserve values; malformed fields record
// errors without changing values. Copies share document nodes and the error list.
class Archive {
  public:
	// Creates a document with a reserved root version key.
	static Archive Save(u32 version = kArchiveVersion);

	// Parses a YAML mapping. Parse errors are available through Errors().
	static Archive Load(std::string_view text);

	// False on parse failure; missing child mappings remain valid but return no fields.
	bool IsValid() const {
		return _valid;
	}

	ArchiveMode Mode() const {
		return _mode;
	}

	bool IsSaving() const {
		return _mode == ArchiveMode::Save;
	}

	bool IsLoading() const {
		return _mode == ArchiveMode::Load;
	}

	// The file version when loading, or the requested version when saving.
	u32 Version() const {
		return _version;
	}

	// Transfers a named value. Missing fields leave it unchanged; returns whether transfer
	// succeeded.
	bool Field(std::string_view key, bool &value);
	bool Field(std::string_view key, i32 &value);
	bool Field(std::string_view key, u32 &value);
	bool Field(std::string_view key, u64 &value);

	// Only resource fields participate in ID remapping.
	bool ResourceField(std::string_view key, u64 &value);

	Archive WithResourceMapper(ResourceIdMapper mapper, void *context) const {
		Archive result = *this;
		result._resourceMapper = mapper;
		result._resourceContext = context;
		return result;
	}

	bool Field(std::string_view key, f32 &value);
	bool Field(std::string_view key, f64 &value);
	bool Field(std::string_view key, std::string &value);
	bool Field(std::string_view key, Vector2 &value);
	bool Field(std::string_view key, Vector3 &value);
	bool Field(std::string_view key, Color &value);

	// Transfers this node directly, for scalar list elements.
	bool Value(bool &value);
	bool Value(i32 &value);
	bool Value(u32 &value);
	bool Value(u64 &value);
	bool Value(f32 &value);
	bool Value(f64 &value);
	bool Value(std::string &value);
	bool Value(Vector2 &value);
	bool Value(Vector3 &value);
	bool Value(Color &value);

	// Serializes enum names, falling back to numbers for unnamed values. Unknown names
	// record an error and leave the value unchanged.
	bool EnumField(std::string_view key, i32 &value, const std::vector<EnumConstant> &constants);
	bool EnumValue(i32 &value, const std::vector<EnumConstant> &constants);

	// Enum storage must be 32-bit, as required by SOWA_REFLECT_ENUM.
	template <typename T>
	bool EnumField(std::string_view key, T &value, const std::vector<EnumConstant> &constants) {
		static_assert(sizeof(T) == sizeof(i32), "enum must have a 32-bit underlying type");
		i32 raw = static_cast<i32>(value);
		const bool transferred = EnumField(key, raw, constants);
		value = static_cast<T>(raw);
		return transferred;
	}

	// --- Nesting ---

	// Creates a nested mapping when saving. Missing mappings return an inert archive;
	// existing non-mapping values record an error.
	Archive Child(std::string_view key);

	// Transfers a sequence through elementFn(Archive&, T&). Loading resizes items first;
	// T must be default-constructible.
	template <typename T, typename Fn>
	bool List(std::string_view key, std::vector<T> &items, Fn &&elementFn) {
		if (!_valid) {
			return false;
		}

		const std::string name(key);
		if (_mode == ArchiveMode::Save) {
			YAML::Node sequence(YAML::NodeType::Sequence);
			for (std::size_t i = 0; i < items.size(); ++i) {
				// Shared node storage lets scalar writes update the sequence element.
				YAML::Node element(YAML::NodeType::Null);
				Archive child(element, _mode, _errors, elementPath(name, i), _version,
					_resourceMapper, _resourceContext);
				elementFn(child, items[i]);
				sequence.push_back(element);
			}
			_node[name] = sequence;
			return true;
		}

		const YAML::Node sequence = lookup(name);
		if (!sequence || sequence.IsNull()) {
			return false;
		}
		if (!sequence.IsSequence()) {
			AddError(key, "expected a sequence");
			return false;
		}

		items.clear();
		items.resize(sequence.size());
		for (std::size_t i = 0; i < sequence.size(); ++i) {
			Archive child(sequence[i], _mode, _errors, elementPath(name, i), _version,
				_resourceMapper, _resourceContext);
			elementFn(child, items[i]);
		}
		return true;
	}

	// Mapping keys in document order. Empty while saving or for a non-mapping node.
	// Allows loading objects whose schema is only known from the file.
	std::vector<std::string> Keys() const;

	// Returns sequence element archives in order while loading. Empty otherwise.
	std::vector<Archive> Elements(std::string_view key);

	// Errors are shared by the root and all child archives.
	bool HasErrors() const {
		return !_errors->empty();
	}

	const std::vector<std::string> &Errors() const {
		return *_errors;
	}

	// Records a problem against key under this archive, for bridges that do
	// their own dispatch (see SerializeComponent).
	void AddError(std::string_view key, std::string_view message);

	// This archive's dotted-slash location in the document ("" at the root),
	// which is what error messages are prefixed with.
	const std::string &Path() const {
		return _path;
	}

	// The written document. Saving only; empty while loading.
	std::string ToString() const;

  private:
	Archive(YAML::Node node, ArchiveMode mode, std::shared_ptr<std::vector<std::string>> errors,
		std::string path, u32 version, ResourceIdMapper mapper = nullptr,
		void *resourceContext = nullptr)
		: _node(node), _mode(mode), _errors(std::move(errors)), _path(std::move(path)),
		  _version(version), _resourceMapper(mapper), _resourceContext(resourceContext) {
	}

	// Reads key through a const view of the node. Going through the
	// non-const operator[] would insert an entry for a key that only ever got
	// looked at, which would then be emitted on save.
	YAML::Node lookup(const std::string &key) const;

	std::string childPath(const std::string &key) const;
	std::string elementPath(const std::string &key, std::size_t index) const;

	// The one place the direction branch lives; every Field/Value overload is
	// a one-line call into it. Defined in the .cpp for the nine field types.
	template <typename T> bool transfer(const std::string *key, T &value);

	bool enumTransfer(const std::string *key, i32 &value,
		const std::vector<EnumConstant> &constants);

	YAML::Node _node;
	ArchiveMode _mode = ArchiveMode::Save;
	std::shared_ptr<std::vector<std::string>> _errors =
		std::make_shared<std::vector<std::string>>();
	std::string _path;
	u32 _version = kArchiveVersion;
	ResourceIdMapper _resourceMapper = nullptr;
	void *_resourceContext = nullptr;
	bool _valid = true;
};

// Document serialization interface. Serialize handles both saving and loading.
class Serializable {
  public:
	virtual ~Serializable() = default;
	virtual void Serialize(Archive &archive) = 0;
};

// Outcome of a whole-file transfer. error covers the file and the parse;
// problems covers individual fields, and can be non-empty on an otherwise
// successful load - that's a file that was read as far as it could be.
struct ArchiveFileResult {
	FsError error = FsError::Ok;

	// False if the bytes were read but aren't a YAML mapping. Kept separate
	// from error so "no such file" stays distinguishable from "the file is
	// corrupt", without putting a parse failure into the filesystem's enum.
	bool parsed = true;

	std::vector<std::string> problems;

	bool Ok() const {
		return static_cast<bool>(*this) && problems.empty();
	}

	explicit operator bool() const {
		return error == FsError::Ok && parsed;
	}
};

ArchiveFileResult SaveToFile(FileSystem &fs, std::string_view path, Serializable &object,
	u32 version = kArchiveVersion);
ArchiveFileResult LoadFromFile(FileSystem &fs, std::string_view path, Serializable &object);

} // namespace sowa
