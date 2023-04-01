#include "sprite2d.hpp"

#include "debug.hpp"

#include "core/renderer.hpp"
#include "core/application.hpp"
#include "scene/scene.hpp"
#include "resource/resource_loader.hpp"

namespace Sowa {
Sprite2D::Sprite2D() {
	_NodeType = "Sprite2D";
}

void Sprite2D::EnterScene() {
}
void Sprite2D::ExitScene() {
}
void Sprite2D::UpdateLogic() {
}
void Sprite2D::UpdateDraw() {
	if (_Texture != nullptr)
		Renderer::get_singleton().DrawTexture(CalculateTransform(), *_Texture.get(), {(float)_Texture->Width(), (float)_Texture->Height()}, 0);
}
} // namespace Sowa
