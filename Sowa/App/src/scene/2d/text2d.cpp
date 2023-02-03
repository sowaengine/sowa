#include "text2d.hpp"

#include "debug.hpp"

#include "core/renderer.hpp"
#include "resource/resource_loader.hpp"

namespace Sowa {
Text2D::Text2D() {
	_NodeType = "Text2D";

	_Font = ResourceLoader::get_singleton().LoadResource<Font>("res://test-font.ttf");
}

void Text2D::EnterScene() {
}
void Text2D::ExitScene() {
}
void Text2D::UpdateLogic() {
}
void Text2D::UpdateDraw() {

	if (_Font != nullptr) {
        Renderer::get_singleton().DrawText(CalculateTransform(), _Text, *_Font.get());
	}
}
} // namespace Sowa
