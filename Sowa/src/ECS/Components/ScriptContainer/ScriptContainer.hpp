#ifndef _E_SCRIPT_CONTAINER_HPP__
#define _E_SCRIPT_CONTAINER_HPP__

#pragma once

#include <string>
#include <vector>

namespace Sowa::Component {
class ScriptContainer {
  public:
	ScriptContainer();
	~ScriptContainer();

	bool RemoveScript(const std::string &id);
	void AddScript(const std::string &id);

  public:
	// holds LuaBehaviour ids
	std::vector<std::string> _LuaScripts{};
};
} // namespace Sowa::Component

#endif