#include "ECS/Systems/Systems.hpp"
#include "ECS/Scene/Scene.hpp"

#include "Core/EngineContext.hpp"

namespace Sowa::Systems {
void ProcessAll(Sowa::Scene *pScene, SystemsFlags flags, EngineContext &ctx, bool pickable /* = false*/) {
	if (flags & SystemsFlags::SpriteRenderer2D)
		Systems::System_Sprite2D(pScene, pickable);

	if (flags & SystemsFlags::Camera2DLogic)
		Systems::System_Camera2DLogic(pScene);
}
} // namespace Sowa::Systems
