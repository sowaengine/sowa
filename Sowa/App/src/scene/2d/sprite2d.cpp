#include "sprite2d.hpp"

#include "debug.hpp"

#include "core/renderer.hpp"
#include "resource/resource_loader.hpp"

namespace Sowa {
Sprite2D::Sprite2D() {
	_NodeType = "Sprite2D";

	_Scale.x = 0.2f;
	_Scale.y = 0.2f;
	_Texture = ResourceLoader::get_singleton().LoadResource<ImageTexture>("res://image.png");
}

void Sprite2D::EnterScene() {
}
void Sprite2D::ExitScene() {
}
void Sprite2D::UpdateLogic() {
}
void Sprite2D::UpdateDraw() {
	static float f = 0.f;
	f += 0.025f;
	_Position.x = std::cos(f) * 200 * std::abs(std::cos(f));
	_Position.y = std::sin(f) * 200 * std::abs(std::sin(f));
	_Rotation += std::sin(f);

	if (_Texture != nullptr)
		Renderer::get_singleton().DrawTexture(_Position, {_Scale.x * _Texture->Width(), _Scale.y * _Texture->Height()}, 0, _Rotation, *_Texture.get(), 0);
}
} // namespace Sowa
