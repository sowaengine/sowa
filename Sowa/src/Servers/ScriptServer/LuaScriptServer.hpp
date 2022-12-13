#ifndef _E_LUASCRIPTSERVER_HPP__
#define _E_LUASCRIPTSERVER_HPP__

#include <memory>
#include <unordered_map>

#include "Core/Application.hpp"
#include "Core/EngineContext.hpp"
#include "Lua/LuaBehaviour.hpp"
#include "Lua/LuaModule.hpp"
#include "Servers/ScriptServer/ScriptServer.hpp"

namespace sol {
class state;
}

namespace Sowa {

class LuaScriptServer : public ScriptServer {
  public:
	std::unordered_map<std::string, LuaBehaviour> &Behaviours() { return _Behaviours; }

  protected:
	friend class Application;
	LuaScriptServer(EngineContext &ctx);
	~LuaScriptServer();

	static LuaScriptServer *CreateServer(EngineContext &ctx) {
		return new LuaScriptServer(ctx);
	}

	void InitModules();
	void UpdateModules();
	void GuiUpdateModules();

	EngineContext &_Context;

  private:
	void LoadModule(const char* path);

	/* implemented in Lua/Classes */

	/**
	 * @brief Register Classes related to ECS
	 * @see Classes/LuaECS.cpp
	 *
	 * @implements Scene, Entity, Components
	 */
	void RegisterECS();

	/**
	 * @brief Register Classes related to Math
	 * @see Classes/LuaMath.cpp
	 *
	 * @implements Vector2
	 */
	void RegisterMath();
	
	/**
	 * @brief Register Resoure classes
	 * @see Classes/LuaResources.cpp
	 *
	 * @implements ImageTexture
	 */
	void RegisterResources();

	/**
	 * @brief Register Singleton Classes
	 * @see Classes/LuaSingleton.cpp
	 *
	 * @implements Application, Wimdow, Debug
	 */
	void RegisterSingleton();

	/**
	 * @see Classes/LuaGui.cpp
	 *
	 * @implements GuiServer
	 */
	void RegisterGuiServer();

	std::unique_ptr<sol::state> _pState{nullptr};

	std::unordered_map<std::string, LuaModule> _Modules{};

	std::unordered_map<std::string, LuaBehaviour> _Behaviours{};
};

} // namespace Sowa

#endif // _E_LUASCRIPTSERVER_HPP__