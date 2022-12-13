#ifndef _E_LUA_BEHAVIOUR_HPP__
#define _E_LUA_BEHAVIOUR_HPP__

#include "sol/sol.hpp"

namespace Sowa {
class Entity;

class LuaBehaviour {
  public:
	LuaBehaviour() = default;
	LuaBehaviour(const std::string &label, const std::string &id);
	~LuaBehaviour();

	void CallStart(Sowa::Entity &entity);
	void CallUpdate(Sowa::Entity &entity);

	std::string &Label() { return _Label; }
	std::string &Id() { return _Id; }

  public:
	sol::function _Start{};
	sol::function _Update{};

  private:
	std::string _Label{""};
	std::string _Id{""};
};
} // namespace Sowa

#endif // _E_LUA_BEHAVIOUR_HPP__