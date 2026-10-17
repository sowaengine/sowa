#include "scene/scene.hpp"
#include "scene/tilemap_scenes.hpp"

#include "core/log.hpp"
#include "ecs/world.hpp"
#include "resource/resource_registry.hpp"
#include "scene/scene_resources.hpp"

namespace sowa {

bool Scene::IsValid() const {
	return _root.IsAlive() && _root.Has<SceneRoot>();
}

std::string Scene::Name() const {
	return IsValid() ? _root.GetName() : std::string();
}

std::string Scene::SourcePath() const {
	if (!IsValid()) {
		return {};
	}

	const SceneRoot *root = _root.Get<SceneRoot>();
	return root != nullptr ? root->sourcePath : std::string();
}

void Scene::SetSourcePath(const std::string &path) {
	if (!IsValid()) {
		return;
	}

	if (SceneRoot *root = _root.Get<SceneRoot>()) {
		root->sourcePath = path;
	}
}

Entity Scene::ActiveCamera() const {
	if (!IsValid()) {
		return Entity();
	}

	const SceneRoot *root = _root.Get<SceneRoot>();
	if (root == nullptr) {
		return Entity();
	}

	// A camera destroyed after being set reads back as null rather than as a
	// stale handle, so callers get one answer to check instead of two.
	return root->activeCamera.IsAlive() ? root->activeCamera : Entity();
}

bool Scene::SetActiveCamera(Entity camera) {
	if (!IsValid()) {
		return false;
	}

	// Clearing is always allowed; anything else has to belong to this scene.
	if (camera.IsAlive() && !camera.IsDescendantOf(_root) && camera != _root) {
		Log::Error("Scene::SetActiveCamera: the camera is not in scene '{}'", Name());
		return false;
	}

	SceneRoot *root = _root.Get<SceneRoot>();
	if (root == nullptr) {
		return false;
	}

	root->activeCamera = camera;
	return true;
}

Entity Scene::CreateEntity(const std::string &name) {
	if (!IsValid()) {
		return Entity();
	}

	return _root.CreateChild(name);
}

namespace {

void setSubtreeEnabled(Entity entity, bool enabled) {
	if (!entity.IsAlive()) {
		return;
	}
	entity.SetEnabled(enabled);
	for (Entity child : entity.GetChildren()) {
		setSubtreeEnabled(child, enabled);
	}
}

} // namespace

void Scene::SetEnabled(bool enabled) {
	if (!IsValid()) {
		return;
	}
	setSubtreeEnabled(_root, enabled);
}

bool Scene::IsEnabled() const {
	return IsValid() && _root.IsEnabled();
}

bool Scene::Own(ResourceRegistry &registry, ResourceID id) {
	if (!IsValid() || !id.IsValid() || Owns(id)) {
		return false;
	}

	// Retained before tracking, so the list never names something the registry
	// doesn't have - which is what made a stale entry possible before.
	if (!registry.Retain(id)) {
		return false;
	}

	if (!_root.Has<SceneResources>()) {
		_root.Add<SceneResources>();
	}

	SceneResources *resources = _root.Get<SceneResources>();
	if (resources == nullptr) {
		return false;
	}

	resources->owned.push_back(id);
	return true;
}

bool Scene::Disown(ResourceRegistry &registry, ResourceID id) {
	if (!IsValid()) {
		return false;
	}

	SceneResources *resources = _root.Get<SceneResources>();
	if (resources == nullptr) {
		return false;
	}

	for (auto it = resources->owned.begin(); it != resources->owned.end(); ++it) {
		if (*it == id) {
			resources->owned.erase(it);
			registry.Release(id);
			return true;
		}
	}
	return false;
}

bool Scene::Owns(ResourceID id) const {
	if (!IsValid()) {
		return false;
	}

	const SceneResources *resources = _root.Get<SceneResources>();
	if (resources == nullptr) {
		return false;
	}

	for (const ResourceID owned : resources->owned) {
		if (owned == id) {
			return true;
		}
	}
	return false;
}

std::vector<ResourceID> Scene::OwnedResources() const {
	if (!IsValid()) {
		return {};
	}

	const SceneResources *resources = _root.Get<SceneResources>();
	return resources != nullptr ? resources->owned : std::vector<ResourceID>();
}

Scene CreateScene(World &world, const std::string &name) {
	Entity root = world.CreateEntity();
	root.SetName(name);
	root.Add(SceneRoot{});
	return Scene(root);
}

Scene SceneOf(Entity entity) {
	for (Entity current = entity; current.IsAlive(); current = current.GetParent()) {
		if (current.Has<SceneRoot>()) {
			return Scene(current);
		}
	}
	return Scene();
}

std::vector<Scene> GetScenes(World &world) {
	std::vector<Scene> scenes;
	// MatchDisabled: a paused editor tab is still an open scene. Default
	// Each would drop it, which is what systems want and what the open list
	// must not do.
	world.EachIncludingDisabled<SceneRoot>([&](Entity entity, SceneRoot &) {
		// A PackedScene instance is never an open scene, even while its
		// parent is still queued (Instantiate inside a system) or when the
		// caller passed no parent.
		if (entity.Has<SceneInstance>()) {
			return;
		}
		// Only top-level roots: a nested SceneRoot that is not an instance
		// (a CreateScene parented by hand) is still part of its host.
		if (entity.HasParent()) {
			return;
		}
		// The snapshot tag distinguishes this disabled duplicate from a paused loaded scene.
		if (entity.Has<SceneSnapshotCopy>()) {
			return;
		}
		scenes.emplace_back(entity);
	});
	return scenes;
}

Scene FindScene(World &world, const std::string &name) {
	const Entity found = world.Find(name);
	if (!found.IsAlive() || !found.Has<SceneRoot>() || found.Has<SceneInstance>()) {
		return Scene();
	}
	return Scene(found);
}

bool IsInstanceRoot(Entity entity) {
	return entity.IsAlive() && entity.Has<SceneInstance>();
}

bool IsInstanceLocked(Entity entity) {
	if (IsGeneratedTileEntity(entity)) {
		return true;
	}
	if (!entity.IsAlive() || entity.Has<SceneInstance>()) {
		return false;
	}
	for (Entity current = entity.GetParent(); current.IsAlive(); current = current.GetParent()) {
		if (current.Has<SceneInstance>()) {
			return true;
		}
	}
	return false;
}

} // namespace sowa
