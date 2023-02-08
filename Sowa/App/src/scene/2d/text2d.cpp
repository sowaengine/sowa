#include "text2d.hpp"

#include "debug.hpp"

#include "core/application.hpp"
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
	Font& fontHandle = _Font != nullptr ? *_Font.get() : Application::get_singleton().GetDefaultFont();
	Renderer::get_singleton().DrawText(CalculateTransform(), _Text, fontHandle);
}
} // namespace Sowa
