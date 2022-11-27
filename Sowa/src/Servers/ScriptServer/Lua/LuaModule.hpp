#ifndef _E_LUA_MODULE_HPP__
#define _E_LUA_MODULE_HPP__

#include "sol/sol.hpp"

#include <string>

namespace Sowa {
class LuaModule {
  public:
	LuaModule();
	~LuaModule();

	inline const std::string &Author() const { return _Author; }
	inline const std::string &Name() const { return _Name; }
	inline int Version() const { return _Version; }

	void CallStart();
	void CallUpdate();
	void CallGuiUpdate();

	void CallOnPrint(const std::string &message);

  private:
	friend class LuaScriptServer;
	std::string _Author{""};
	std::string _Name{""};
	int _Version{-1};

	sol::function _Start{};
	sol::function _Update{};
	sol::function _GuiUpdate{};

	sol::function _OnPrint{};
};
} // namespace Sowa

#endif // _E_LUA_MODULE_HPP__