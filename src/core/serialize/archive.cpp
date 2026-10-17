#include "core/serialize/archive.hpp"

#include <cstring>
#include <utility>

#include <fmt/format.h>

#include "core/filesystem/file_system.hpp"

namespace sowa {

namespace {

void pushError(std::vector<std::string> &errors, const std::string &path, std::string message) {
	errors.push_back(path.empty() ? std::move(message) : fmt::format("{}: {}", path, message));
}

// --- Scalars ---

template <typename T>
bool decodeScalar(const YAML::Node &node, T &value, std::vector<std::string> &errors,
	const std::string &path, const char *expected) {
	if (!node.IsScalar()) {
		pushError(errors, path, fmt::format("expected {}", expected));
		return false;
	}

	// convert<T>::decode rather than node.as<T>(): same conversion, but it
	// reports failure by returning false instead of throwing, and a malformed
	// field is an ordinary outcome here rather than an exceptional one.
	T parsed{};
	if (!YAML::convert<T>::decode(node, parsed)) {
		pushError(errors, path, fmt::format("expected {}, got \"{}\"", expected, node.Scalar()));
		return false;
	}

	value = parsed;
	return true;
}

// Store vectors and colors as flow sequences in component field order.

YAML::Node encodeFloats(const f32 *values, std::size_t count) {
	YAML::Node node(YAML::NodeType::Sequence);
	for (std::size_t i = 0; i < count; ++i) {
		node.push_back(values[i]);
	}
	node.SetStyle(YAML::EmitterStyle::Flow);
	return node;
}

// Decode into caller-owned scratch storage; commit only after every element succeeds.
bool decodeFloats(const YAML::Node &node, f32 *out, std::size_t minCount, std::size_t maxCount,
	std::vector<std::string> &errors, const std::string &path, const char *expected) {
	if (!node.IsSequence() || node.size() < minCount || node.size() > maxCount) {
		pushError(errors, path, fmt::format("expected {}", expected));
		return false;
	}

	for (std::size_t i = 0; i < node.size(); ++i) {
		if (!YAML::convert<f32>::decode(node[i], out[i])) {
			pushError(errors, path,
				fmt::format("expected {}, element {} is not a number", expected, i));
			return false;
		}
	}
	return true;
}

// --- Per-type encode/decode, selected by overload from Archive::transfer ---

YAML::Node encodeValue(const bool &value) {
	return YAML::Node(value);
}

YAML::Node encodeValue(const i32 &value) {
	return YAML::Node(value);
}

YAML::Node encodeValue(const u32 &value) {
	return YAML::Node(value);
}

YAML::Node encodeValue(const u64 &value) {
	return YAML::Node(value);
}

YAML::Node encodeValue(const f32 &value) {
	return YAML::Node(value);
}

YAML::Node encodeValue(const f64 &value) {
	return YAML::Node(value);
}

YAML::Node encodeValue(const std::string &value) {
	return YAML::Node(value);
}

YAML::Node encodeValue(const Vector2 &value) {
	const f32 components[]{value.x, value.y};
	return encodeFloats(components, 2);
}

YAML::Node encodeValue(const Vector3 &value) {
	const f32 components[]{value.x, value.y, value.z};
	return encodeFloats(components, 3);
}

YAML::Node encodeValue(const Color &value) {
	const f32 components[]{value.r, value.g, value.b, value.a};
	return encodeFloats(components, 4);
}

bool decodeValue(const YAML::Node &node, bool &value, std::vector<std::string> &errors,
	const std::string &path) {
	return decodeScalar(node, value, errors, path, "a boolean");
}

bool decodeValue(const YAML::Node &node, i32 &value, std::vector<std::string> &errors,
	const std::string &path) {
	return decodeScalar(node, value, errors, path, "an integer");
}

bool decodeValue(const YAML::Node &node, u32 &value, std::vector<std::string> &errors,
	const std::string &path) {
	return decodeScalar(node, value, errors, path, "an unsigned integer");
}

bool decodeValue(const YAML::Node &node, u64 &value, std::vector<std::string> &errors,
	const std::string &path) {
	return decodeScalar(node, value, errors, path, "an unsigned integer");
}

bool decodeValue(const YAML::Node &node, f32 &value, std::vector<std::string> &errors,
	const std::string &path) {
	return decodeScalar(node, value, errors, path, "a number");
}

bool decodeValue(const YAML::Node &node, f64 &value, std::vector<std::string> &errors,
	const std::string &path) {
	return decodeScalar(node, value, errors, path, "a number");
}

bool decodeValue(const YAML::Node &node, std::string &value, std::vector<std::string> &errors,
	const std::string &path) {
	return decodeScalar(node, value, errors, path, "a string");
}

bool decodeValue(const YAML::Node &node, Vector2 &value, std::vector<std::string> &errors,
	const std::string &path) {
	f32 components[4]{value.x, value.y, 0.f, 0.f};
	if (!decodeFloats(node, components, 2, 2, errors, path, "[x, y]")) {
		return false;
	}
	value = Vector2(components[0], components[1]);
	return true;
}

bool decodeValue(const YAML::Node &node, Vector3 &value, std::vector<std::string> &errors,
	const std::string &path) {
	f32 components[4]{value.x, value.y, value.z, 0.f};
	if (!decodeFloats(node, components, 3, 3, errors, path, "[x, y, z]")) {
		return false;
	}
	value = Vector3(components[0], components[1], components[2]);
	return true;
}

bool decodeValue(const YAML::Node &node, Color &value, std::vector<std::string> &errors,
	const std::string &path) {
	// Alpha seeded opaque rather than from the current value, so a
	// three-element [r, g, b] means exactly one thing regardless of what the
	// target happened to hold.
	f32 components[4]{value.r, value.g, value.b, 1.f};
	if (!decodeFloats(node, components, 3, 4, errors, path, "[r, g, b] or [r, g, b, a]")) {
		return false;
	}
	value = Color(components[0], components[1], components[2], components[3]);
	return true;
}

// --- Enum constants ---

const EnumConstant *findConstantByValue(const std::vector<EnumConstant> &constants, i32 value) {
	for (const EnumConstant &constant : constants) {
		if (constant.value == value) {
			return &constant;
		}
	}
	return nullptr;
}

const EnumConstant *findConstantByName(const std::vector<EnumConstant> &constants,
	const std::string &name) {
	for (const EnumConstant &constant : constants) {
		if (constant.name == name) {
			return &constant;
		}
	}
	return nullptr;
}

std::string joinConstantNames(const std::vector<EnumConstant> &constants) {
	std::string joined;
	for (const EnumConstant &constant : constants) {
		if (!joined.empty()) {
			joined += ", ";
		}
		joined += constant.name;
	}
	return joined;
}

constexpr std::string_view kVersionKey = "version";

} // namespace

Archive Archive::Save(u32 version) {
	YAML::Node root(YAML::NodeType::Map);
	root[std::string(kVersionKey)] = version;
	return Archive(root, ArchiveMode::Save, std::make_shared<std::vector<std::string>>(),
		std::string(), version);
}

Archive Archive::Load(std::string_view text) {
	auto errors = std::make_shared<std::vector<std::string>>();

	YAML::Node root;
	try {
		root = YAML::Load(std::string(text));
	} catch (const YAML::Exception &exception) {
		errors->push_back(fmt::format("failed to parse: {}", exception.what()));
		Archive archive(YAML::Node(), ArchiveMode::Load, errors, std::string(), 0);
		archive._valid = false;
		return archive;
	}

	if (!root || root.IsNull() || !root.IsMap()) {
		errors->push_back("expected a YAML mapping at the document root");
		Archive archive(YAML::Node(), ArchiveMode::Load, errors, std::string(), 0);
		archive._valid = false;
		return archive;
	}

	// A file written before versioning existed has no key and reads back as
	// 0, which is exactly the signal a future migration wants.
	u32 version = 0;
	const YAML::Node &constRoot = root;
	if (const YAML::Node versionNode = constRoot[std::string(kVersionKey)];
		versionNode && versionNode.IsScalar()) {
		YAML::convert<u32>::decode(versionNode, version);
	}

	return Archive(root, ArchiveMode::Load, errors, std::string(), version);
}

YAML::Node Archive::lookup(const std::string &key) const {
	const YAML::Node &self = _node;
	if (!self || self.IsNull() || !self.IsMap()) {
		return YAML::Node();
	}

	// Normalize missing yaml-cpp nodes: inspecting a missing-key node can throw InvalidNode.
	const YAML::Node found = self[key];
	if (!found) {
		return YAML::Node();
	}
	return found;
}

std::string Archive::childPath(const std::string &key) const {
	return _path.empty() ? key : fmt::format("{}/{}", _path, key);
}

std::string Archive::elementPath(const std::string &key, std::size_t index) const {
	return fmt::format("{}[{}]", childPath(key), index);
}

template <typename T> bool Archive::transfer(const std::string *key, T &value) {
	if (!_valid) {
		return false;
	}

	if (_mode == ArchiveMode::Save) {
		const YAML::Node encoded = encodeValue(value);
		if (key != nullptr) {
			_node[*key] = encoded;
		} else {
			_node = encoded;
		}
		return true;
	}

	const YAML::Node node = key != nullptr ? lookup(*key) : _node;
	if (!node || node.IsNull()) {
		return false;
	}

	return decodeValue(node, value, *_errors, key != nullptr ? childPath(*key) : _path);
}

#define SOWA_ARCHIVE_TRANSFER(Type)                                                                \
	bool Archive::Field(std::string_view key, Type &value) {                                       \
		const std::string name(key);                                                               \
		return transfer(&name, value);                                                             \
	}                                                                                              \
	bool Archive::Value(Type &value) {                                                             \
		return transfer<Type>(nullptr, value);                                                     \
	}

SOWA_ARCHIVE_TRANSFER(bool)
SOWA_ARCHIVE_TRANSFER(i32)
SOWA_ARCHIVE_TRANSFER(u32)
SOWA_ARCHIVE_TRANSFER(u64)
SOWA_ARCHIVE_TRANSFER(f32)
SOWA_ARCHIVE_TRANSFER(f64)
SOWA_ARCHIVE_TRANSFER(std::string)
SOWA_ARCHIVE_TRANSFER(Vector2)
SOWA_ARCHIVE_TRANSFER(Vector3)
SOWA_ARCHIVE_TRANSFER(Color)

#undef SOWA_ARCHIVE_TRANSFER

bool Archive::EnumField(std::string_view key, i32 &value,
	const std::vector<EnumConstant> &constants) {
	const std::string name(key);
	return enumTransfer(&name, value, constants);
}

bool Archive::EnumValue(i32 &value, const std::vector<EnumConstant> &constants) {
	return enumTransfer(nullptr, value, constants);
}

bool Archive::enumTransfer(const std::string *key, i32 &value,
	const std::vector<EnumConstant> &constants) {
	if (!_valid) {
		return false;
	}

	if (_mode == ArchiveMode::Save) {
		// An unnamed value (a cast from an arbitrary int) is written as its
		// number so it still round-trips, rather than being dropped.
		const EnumConstant *constant = findConstantByValue(constants, value);
		const YAML::Node encoded =
			constant != nullptr ? YAML::Node(constant->name) : YAML::Node(value);
		if (key != nullptr) {
			_node[*key] = encoded;
		} else {
			_node = encoded;
		}
		return true;
	}

	const YAML::Node node = key != nullptr ? lookup(*key) : _node;
	if (!node || node.IsNull()) {
		return false;
	}

	const std::string path = key != nullptr ? childPath(*key) : _path;
	if (!node.IsScalar()) {
		pushError(*_errors, path, "expected an enum name");
		return false;
	}

	const std::string &text = node.Scalar();
	if (const EnumConstant *constant = findConstantByName(constants, text)) {
		value = constant->value;
		return true;
	}

	// Falls back to a raw number, which is what the save side writes for a
	// value no constant covers.
	i32 raw = 0;
	if (YAML::convert<i32>::decode(node, raw)) {
		value = raw;
		return true;
	}

	pushError(*_errors, path,
		fmt::format("\"{}\" is not one of [{}]", text, joinConstantNames(constants)));
	return false;
}

Archive Archive::Child(std::string_view key) {
	const std::string name(key);
	const std::string path = childPath(name);

	if (!_valid) {
		return *this;
	}

	if (_mode == ArchiveMode::Save) {
		// Reuse existing mappings. lookup handles missing keys without invalid YAML nodes.
		const YAML::Node existing = lookup(name);
		if (!existing.IsMap()) {
			_node[name] = YAML::Node(YAML::NodeType::Map);
		}
		return Archive(_node[name], _mode, _errors, path, _version);
	}

	const YAML::Node node = lookup(name);
	if (!node || node.IsNull()) {
		// Inert rather than invalid: nested reads all return false and leave
		// their targets at whatever defaults the object already had, which is
		// the same "absence isn't an error" rule one level down.
		return Archive(YAML::Node(), _mode, _errors, path, _version);
	}
	if (!node.IsMap()) {
		pushError(*_errors, path, "expected a mapping");
		return Archive(YAML::Node(), _mode, _errors, path, _version);
	}

	return Archive(node, _mode, _errors, path, _version);
}

std::vector<std::string> Archive::Keys() const {
	std::vector<std::string> keys;
	if (!_valid || _mode != ArchiveMode::Load || !_node || !_node.IsMap()) {
		return keys;
	}

	for (const auto &entry : _node) {
		keys.push_back(entry.first.as<std::string>());
	}
	return keys;
}

std::vector<Archive> Archive::Elements(std::string_view key) {
	std::vector<Archive> elements;
	if (!_valid || _mode != ArchiveMode::Load) {
		return elements;
	}

	const std::string name(key);
	const YAML::Node sequence = lookup(name);
	if (!sequence || sequence.IsNull()) {
		return elements;
	}
	if (!sequence.IsSequence()) {
		AddError(name, "expected a sequence");
		return elements;
	}

	elements.reserve(sequence.size());
	for (std::size_t i = 0; i < sequence.size(); ++i) {
		// push_back, not emplace_back: the constructor is private, so the
		// allocator can't reach it but this member function can.
		elements.push_back(Archive(sequence[i], _mode, _errors, elementPath(name, i), _version));
	}
	return elements;
}

void Archive::AddError(std::string_view key, std::string_view message) {
	pushError(*_errors, childPath(std::string(key)), std::string(message));
}

std::string Archive::ToString() const {
	if (_mode != ArchiveMode::Save || !_valid) {
		return {};
	}
	return YAML::Dump(_node);
}

ArchiveFileResult SaveToFile(FileSystem &fs, std::string_view path, Serializable &object,
	u32 version) {
	Archive archive = Archive::Save(version);
	object.Serialize(archive);

	const std::string text = archive.ToString();
	FileData data;
	data.bytes.resize(text.size());
	if (!text.empty()) {
		std::memcpy(data.bytes.data(), text.data(), text.size());
	}

	ArchiveFileResult result;
	result.problems = archive.Errors();
	result.error = fs.Save(path, data);
	return result;
}

ArchiveFileResult LoadFromFile(FileSystem &fs, std::string_view path, Serializable &object) {
	ArchiveFileResult result;

	auto loaded = fs.Load(path);
	if (!loaded || !loaded.value) {
		result.error = loaded.error == FsError::Ok ? FsError::IoError : loaded.error;
		return result;
	}

	const std::string text(reinterpret_cast<const char *>(loaded.value->Data()),
		loaded.value->Size());

	Archive archive = Archive::Load(text);
	if (!archive.IsValid()) {
		result.parsed = false;
		result.problems = archive.Errors();
		return result;
	}

	object.Serialize(archive);
	result.problems = archive.Errors();
	return result;
}

} // namespace sowa
