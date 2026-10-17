#include "ecs/components/transform2d.hpp"

#include <vector>

#include "math/matrix.hpp"

namespace sowa {

glm::mat4 Transform2D::LocalMatrix() const {
	return Matrix::CalculateTransform2D(position, rotation, scale);
}

Vector2 Transform2D::WorldPosition() const {
	return Vector2(worldMatrix[3][0], worldMatrix[3][1]);
}

f32 Transform2D::WorldRotation() const {
	f32 result = 0.f;
	Matrix::DecomposeTransform2D(worldMatrix, nullptr, &result, nullptr);
	return result;
}

Vector2 Transform2D::WorldScale() const {
	Vector2 result(1.f, 1.f);
	Matrix::DecomposeTransform2D(worldMatrix, nullptr, nullptr, &result);
	return result;
}

glm::mat4 ComputeWorldMatrix(Entity entity) {
	// Collected child-first, then composed root-first - the same order the
	// cascade pass produces, so both paths agree to the bit.
	std::vector<const Transform2D *> chain;
	for (Entity current = entity; current.IsAlive(); current = current.GetParent()) {
		if (const Transform2D *transform = current.Get<Transform2D>()) {
			chain.push_back(transform);
		}
	}

	glm::mat4 result(1.f);
	for (auto it = chain.rbegin(); it != chain.rend(); ++it) {
		result = Matrix::CalculateTransform2D((*it)->position, (*it)->rotation, (*it)->scale,
			Vector2(0.f, 0.f), result);
	}
	return result;
}

bool Reparent(Entity child, Entity newParent) {
	if (!child.IsAlive()) {
		return false;
	}

	// Nothing to preserve without a transform; this is a plain reparent.
	if (!child.Has<Transform2D>()) {
		if (!newParent.IsAlive()) {
			child.ClearParent();
			return true;
		}
		return child.SetParent(newParent);
	}

	const glm::mat4 world = ComputeWorldMatrix(child);

	if (newParent.IsAlive()) {
		if (!child.SetParent(newParent)) {
			return false;
		}
	} else {
		child.ClearParent();
	}

	// The parent's own chain is walked *after* the move: reparenting changes
	// which ancestors contribute, and ComputeWorldMatrix(newParent) is the
	// answer either way since a valid reparent can't put newParent under child.
	const glm::mat4 parentWorld =
		newParent.IsAlive() ? ComputeWorldMatrix(newParent) : glm::mat4(1.f);
	const glm::mat4 local = glm::inverse(parentWorld) * world;

	// Re-Get, not reuse: parenting adds a ChildOf pair, which moves the entity
	// to a different table and invalidates any pointer taken before the move.
	Transform2D *transform = child.Get<Transform2D>();
	if (transform == nullptr) {
		return true;
	}

	// A degenerate ancestor (zero scale) has no meaningful inverse, so the
	// decomposition of an entity under one is garbage in and garbage out. Not
	// guarded: the entity was already collapsed to nothing before the move.
	Matrix::DecomposeTransform2D(local, &transform->position, &transform->rotation,
		&transform->scale);
	transform->worldMatrix = world;
	return true;
}

} // namespace sowa
