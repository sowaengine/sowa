#include "Resource/Resource.hpp"
#include "Servers/ScriptServer/LuaScriptServer.hpp"

namespace Sowa {
void LuaScriptServer::RegisterResources() {
	_pState->new_enum(
		"Resource",

		"ImageTexture", Resource_ImageTexture);

	_pState->new_usertype<ImageTexture>(
		"ImageTexture");
}
} // namespace Sowa
