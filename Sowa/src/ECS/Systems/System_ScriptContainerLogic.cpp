#include "ECS/Components/Components.hpp"
#include "ECS/Scene/Scene.hpp"
#include "ECS/Systems/Systems.hpp"

#include "Core/Application.hpp"
#include "Servers/ScriptServer/LuaScriptServer.hpp"

namespace Sowa::Systems {
void System_ScriptContainerLogic(Sowa::Scene *pScene, EngineContext &ctx) {
	Application &app = *(ctx.GetSingleton<Application>(Server::APPLICATION));
	LuaScriptServer &luaServer = *(ctx.GetSingleton<LuaScriptServer>(Server::SCRIPTSERVER_LUA));

	auto view = app.GetCurrentScene()->m_Registry.view<Component::ScriptContainer>();
	for (auto &e : view) {
		Entity entity(e, &app.GetCurrentScene()->m_Registry);
		auto &scriptContainer = entity.GetComponent<Component::ScriptContainer>();

		for (auto &[id, behaviour] : luaServer.Behaviours()) {
			for (auto scriptId : scriptContainer._LuaScripts) {
				if (scriptId == id) {
					behaviour.CallUpdate(entity);
				}
			}
		}
	}
}
} // namespace Sowa::Systems
