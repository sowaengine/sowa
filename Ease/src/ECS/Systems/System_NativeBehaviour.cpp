#include "ECS/Scene/Scene.hpp"
#include "ECS/Systems/Systems.hpp"

#include "ECS/Components/NativeBehaviourClass/NativeBehaviourClass.hpp"

#include "Core/Application.hpp"

namespace Ease::Systems {
void System_NativeBehaviour(Ease::Scene *pScene) {
	auto view = pScene->m_Registry.view<Component::NativeBehaviourClass>();
	for (auto &e : view) {
		Entity entity(e, &pScene->m_Registry);

		Component::NativeBehaviourClass &nbehaviour = entity.GetComponent<Component::NativeBehaviourClass>();

		if (!nbehaviour.Factory()) {
			nbehaviour.Factory() = Application::get_singleton().GetFactory(nbehaviour.ClassName());

			if (nbehaviour.Factory()) {
				nbehaviour.Behaviour() = nbehaviour.Factory()->Create();
				if (nbehaviour.Behaviour()) {
					nbehaviour.Behaviour()->self = entity;
					nbehaviour.Behaviour()->Start();
				}
			}
		}

		if (nbehaviour.Behaviour())
			nbehaviour.Behaviour()->Update();
	}
}
} // namespace Ease::Systems
