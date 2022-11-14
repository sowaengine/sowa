#include "Resource/Resource.hpp"
#include "Servers/ScriptServer/LuaScriptServer.hpp"

namespace Ease {
void LuaScriptServer::RegisterResources() {
	_pState->new_enum(
		"Resource",

		"ImageTexture", Resource_ImageTexture);

	_pState->new_usertype<ImageTexture>(
		"ImageTexture");
}
} // namespace Ease
