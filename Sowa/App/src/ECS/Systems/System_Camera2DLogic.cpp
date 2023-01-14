#include "ECS/Scene/Scene.hpp"
#include "ECS/Systems/Systems.hpp"

#include "ECS/Components/Button/Button.hpp"
#include "ECS/Components/UITransform/UITransform.hpp"

#include "Core/Application.hpp"
#include "Core/Input.hpp"

namespace Sowa::Systems {
void System_Camera2DLogic(Sowa::Scene *pScene) {
	Sowa::Application &app = Sowa::Application::get_singleton();

	bool found = false;
	auto view = app.GetCurrentScene()->m_Registry.view<Component::Camera2D, Component::Transform2D>();
	for (auto &e : view) {
		Entity entity(e, &app.GetCurrentScene()->m_Registry);
		auto &transformc = entity.GetComponent<Component::Transform2D>();
		auto &camerac = entity.GetComponent<Component::Camera2D>();

		if (camerac.Current()) {
			pScene->CurrentCamera2D() = camerac;
			pScene->CurrentCameraTransform2D() = transformc;
			found = true;
		}
	}

	if (!found) {
		pScene->CurrentCamera2D() = Component::Camera2D();
		pScene->CurrentCameraTransform2D() = Component::Transform2D();
	}
}
} // namespace Sowa::Systems
