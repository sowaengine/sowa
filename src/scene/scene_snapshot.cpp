#include "scene/scene_snapshot.hpp"

#include <string>
#include <vector>

#include "core/log.hpp"
#include "core/reflection/field_ref.hpp"
#include "core/reflection/field_type.hpp"
#include "core/types.hpp"
#include "math/color.hpp"
#include "math/vector2.hpp"
#include "math/vector3.hpp"
#include "resource/resource.hpp"
#include "resource/resource_id.hpp"

namespace sowa {

namespace {

struct StoredField {
	FieldType type = FieldType::F32;
	bool b = false;
	i32 i = 0;
	u32 u = 0;
	f32 f = 0;
	f64 d = 0;
	std::string s;
	Vector2 v2;
	Vector3 v3;
	Color color;
	u64 id = 0;
};

struct CapturedResource {
	ResourceID id;
	std::vector<StoredField> fields;
};

// Unregistered on purpose, like SceneResources: the entity walk would
// otherwise write a play-mode snapshot into the scene file, and a list of
// captured fields is nothing reflection can describe anyway.
struct SceneSnapshot {
	Entity copy;
	std::vector<ResourceID> retained;
	std::vector<CapturedResource> captured;
};

FieldRef fieldAt(Resource &resource, const FieldInfo &field) {
	return FieldRef(static_cast<u8 *>(static_cast<void *>(&resource)) + field.offset, &field);
}

StoredField readField(const FieldRef &ref) {
	StoredField out;
	if (!ref.IsValid()) {
		return out;
	}

	out.type = ref.Type();
	switch (out.type) {
	case FieldType::Bool:
		if (const bool *value = ref.As<bool>()) {
			out.b = *value;
		}
		break;
	case FieldType::I32:
		if (const i32 *value = ref.As<i32>()) {
			out.i = *value;
		}
		break;
	case FieldType::U32:
		if (const u32 *value = ref.As<u32>()) {
			out.u = *value;
		}
		break;
	case FieldType::F32:
		if (const f32 *value = ref.As<f32>()) {
			out.f = *value;
		}
		break;
	case FieldType::F64:
		if (const f64 *value = ref.As<f64>()) {
			out.d = *value;
		}
		break;
	case FieldType::String:
		if (const std::string *value = ref.As<std::string>()) {
			out.s = *value;
		}
		break;
	case FieldType::Vector2:
		if (const Vector2 *value = ref.As<Vector2>()) {
			out.v2 = *value;
		}
		break;
	case FieldType::Vector3:
		if (const Vector3 *value = ref.As<Vector3>()) {
			out.v3 = *value;
		}
		break;
	case FieldType::Color:
		if (const Color *value = ref.As<Color>()) {
			out.color = *value;
		}
		break;
	case FieldType::Enum:
		out.i = *static_cast<i32 *>(ref.Data());
		break;
	case FieldType::ResourceRef:
		out.id = *static_cast<u64 *>(ref.Data());
		break;
	case FieldType::EntityRef:
		// Resources can't hold entity refs (ResourceTypeBuilder refuses them).
		break;
	}
	return out;
}

void writeField(const FieldRef &ref, const StoredField &stored) {
	if (!ref.IsValid() || ref.Type() != stored.type) {
		return;
	}

	switch (stored.type) {
	case FieldType::Bool:
		if (bool *value = ref.As<bool>()) {
			*value = stored.b;
		}
		break;
	case FieldType::I32:
		if (i32 *value = ref.As<i32>()) {
			*value = stored.i;
		}
		break;
	case FieldType::U32:
		if (u32 *value = ref.As<u32>()) {
			*value = stored.u;
		}
		break;
	case FieldType::F32:
		if (f32 *value = ref.As<f32>()) {
			*value = stored.f;
		}
		break;
	case FieldType::F64:
		if (f64 *value = ref.As<f64>()) {
			*value = stored.d;
		}
		break;
	case FieldType::String:
		if (std::string *value = ref.As<std::string>()) {
			*value = stored.s;
		}
		break;
	case FieldType::Vector2:
		if (Vector2 *value = ref.As<Vector2>()) {
			*value = stored.v2;
		}
		break;
	case FieldType::Vector3:
		if (Vector3 *value = ref.As<Vector3>()) {
			*value = stored.v3;
		}
		break;
	case FieldType::Color:
		if (Color *value = ref.As<Color>()) {
			*value = stored.color;
		}
		break;
	case FieldType::Enum:
		*static_cast<i32 *>(ref.Data()) = stored.i;
		break;
	case FieldType::ResourceRef:
		*static_cast<u64 *>(ref.Data()) = stored.id;
		break;
	case FieldType::EntityRef:
		break;
	}
}

bool fieldsEqual(const StoredField &a, const StoredField &b) {
	if (a.type != b.type) {
		return false;
	}
	switch (a.type) {
	case FieldType::Bool:
		return a.b == b.b;
	case FieldType::I32:
	case FieldType::Enum:
		return a.i == b.i;
	case FieldType::U32:
		return a.u == b.u;
	case FieldType::F32:
		return a.f == b.f;
	case FieldType::F64:
		return a.d == b.d;
	case FieldType::String:
		return a.s == b.s;
	case FieldType::Vector2:
		return a.v2 == b.v2;
	case FieldType::Vector3:
		return a.v3 == b.v3;
	case FieldType::Color:
		return a.color == b.color;
	case FieldType::ResourceRef:
		return a.id == b.id;
	case FieldType::EntityRef:
		return true;
	}
	return true;
}

std::vector<StoredField> captureFields(Resource &resource) {
	std::vector<StoredField> fields;
	fields.reserve(resource.Fields().size());
	for (const FieldInfo &field : resource.Fields()) {
		fields.push_back(readField(fieldAt(resource, field)));
	}
	return fields;
}

bool restoreFields(Resource &resource, const std::vector<StoredField> &stored) {
	const std::vector<FieldInfo> &fields = resource.Fields();
	if (stored.size() != fields.size()) {
		return false;
	}

	bool moved = false;
	for (size_t i = 0; i < fields.size(); ++i) {
		if (!fieldsEqual(readField(fieldAt(resource, fields[i])), stored[i])) {
			moved = true;
			break;
		}
	}
	if (!moved) {
		return false;
	}

	for (size_t i = 0; i < fields.size(); ++i) {
		writeField(fieldAt(resource, fields[i]), stored[i]);
	}
	return true;
}

void destroySnapshot(SceneSnapshot &snapshot, ResourceRegistry &resources) {
	for (const ResourceID id : snapshot.retained) {
		resources.Release(id);
	}
	snapshot.retained.clear();
	snapshot.captured.clear();
	if (snapshot.copy.IsAlive()) {
		snapshot.copy.Destroy();
	}
}

} // namespace

bool TakeSnapshot(Scene scene, ResourceRegistry &resources) {
	if (!scene.IsValid()) {
		return false;
	}

	Entity root = scene.Root();
	if (root.Has<SceneSnapshot>()) {
		Log::Error("TakeSnapshot: scene '{}' already has a snapshot", scene.Name());
		return false;
	}

	Entity copy = root.Duplicate();
	if (!copy.IsAlive()) {
		return false;
	}

	// Tag first so GetScenes (which now matches disabled roots) never sees
	// this as an open scene, even briefly. Restore removes it.
	copy.Add<SceneSnapshotCopy>();

	// Queries skip EcsDisabled by default, but only on entities that carry
	// it - disabling the root does not hide its children. Scene::SetEnabled
	// is that walk.
	Scene(copy).SetEnabled(false);

	// Clear the duplicate root name so snapshot paths cannot resolve; preserve child
	// names for restoration under the original scene path.
	copy.SetName("");

	SceneSnapshot snapshot;
	snapshot.copy = copy;
	for (const ResourceID id : scene.OwnedResources()) {
		Ref<Resource> resource = resources.Get(id);
		if (!resource) {
			continue;
		}
		// Extra claim: the live scene's Own is one, and this is another, so a
		// Disown during play drops the scene's claim without destroying the
		// object the field buffer has to copy back onto.
		if (!resources.Retain(id)) {
			continue;
		}
		snapshot.retained.push_back(id);
		snapshot.captured.push_back(CapturedResource{id, captureFields(*resource)});
	}

	root.Add(snapshot);
	return true;
}

bool RestoreSnapshot(Scene &scene, ResourceRegistry &resources) {
	if (!scene.IsValid()) {
		return false;
	}

	SceneSnapshot *snapshot = scene.Root().Get<SceneSnapshot>();
	if (snapshot == nullptr || !snapshot->copy.IsAlive()) {
		Log::Error("RestoreSnapshot: scene '{}' has no snapshot", scene.Name());
		return false;
	}

	// Fields first, while the objects are still guaranteed to exist. Reload
	// only where a field actually moved: an ImageTexture Reload is a decode
	// and a GPU upload, and Stop shouldn't pay that for every texture.
	for (const CapturedResource &captured : snapshot->captured) {
		Ref<Resource> resource = resources.Get(captured.id);
		if (!resource) {
			continue;
		}
		if (restoreFields(*resource, captured.fields)) {
			resources.Reload(captured.id);
		}
	}

	Entity copy = snapshot->copy;
	const std::string name = scene.Name();

	// Release live-tree resources before destruction. Snapshot retains become restored-scene
	// ownership.
	for (const ResourceID id : scene.OwnedResources()) {
		resources.Release(id);
	}

	scene.Root().Destroy();

	copy.Remove<SceneSnapshotCopy>();
	copy.SetName(name);
	Scene(copy).SetEnabled(true);
	scene = Scene(copy);
	return true;
}

void DiscardSnapshot(Scene scene, ResourceRegistry &resources) {
	if (!scene.IsValid()) {
		return;
	}

	SceneSnapshot *snapshot = scene.Root().Get<SceneSnapshot>();
	if (snapshot == nullptr) {
		return;
	}

	destroySnapshot(*snapshot, resources);
	scene.Root().Remove<SceneSnapshot>();
}

} // namespace sowa
