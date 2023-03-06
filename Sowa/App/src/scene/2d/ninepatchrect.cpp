#include "ninepatchrect.hpp"

#include "debug.hpp"

#include "core/application.hpp"
#include "core/renderer.hpp"
#include "resource/resource_loader.hpp"
#include "scene/scene.hpp"

namespace Sowa {
NinePatchRect::NinePatchRect() {
	_NodeType = "NinePatchRect";
}

void NinePatchRect::EnterScene() {
}
void NinePatchRect::ExitScene() {
}
void NinePatchRect::UpdateLogic() {
}
void NinePatchRect::UpdateDraw() {
	if (_Texture != nullptr) {
		Renderer::get_singleton().DrawTexture(CalculateTransform(), *_Texture.get(), {(float)_Texture->Width(), (float)_Texture->Height()}, 0);
	}
}
} // namespace Sowa
