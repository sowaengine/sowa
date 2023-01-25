#include "text2d.hpp"

#include "debug.hpp"

#include "core/renderer.hpp"
#include "resource/resource_loader.hpp"

namespace Sowa {
Text2D::Text2D() {
	_NodeType = "Text2D";

	_Scale.x = 0.2f;
	_Scale.y = 0.2f;
	_Font = ResourceLoader::get_singleton().LoadResource<Font>("res://test-font.ttf");
}

void Text2D::EnterScene() {
}
void Text2D::ExitScene() {
}
void Text2D::UpdateLogic() {
}
void Text2D::UpdateDraw() {
	static float f = 0.f;
	f += 0.025f;
	_Position.x = std::cos(f) * 200 * std::abs(std::cos(f));
	_Position.y = -(std::sin(f) * 200 * std::abs(std::sin(f)));
	_Rotation += std::sin(f);

	if (_Font != nullptr)
        Renderer::get_singleton().DrawText(_Position, 1.f, _Rotation, _Text, *_Font.get());
}
} // namespace Sowa
