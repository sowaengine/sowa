#include "ECS/Components/ScriptContainer/ScriptContainer.hpp"

namespace Sowa::Component {
ScriptContainer::ScriptContainer() {
}

ScriptContainer::~ScriptContainer() {
}

bool ScriptContainer::RemoveScript(const std::string &id) {
	for (size_t i = 0; i < _LuaScripts.size(); i++) {
		if (id == _LuaScripts[i]) {
			_LuaScripts.erase(_LuaScripts.begin() + i);
			return true;
		}
	}

	return false;
}

void ScriptContainer::AddScript(const std::string &id) {
	_LuaScripts.push_back(id);
}
} // namespace Sowa::Component