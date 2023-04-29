#include "button.hpp"

#include "debug.hpp"
#include "core/renderer.hpp"
#include "core/application.hpp"

#include "glm/gtc/matrix_transform.hpp"

namespace sowa {
Button::Button() {
	_NodeType = "Button";
	m_type = Typename();
}

void Button::EnterScene() {
}
void Button::ExitScene() {
}
void Button::UpdateLogic() {
}
void Button::UpdateDraw() {
    glm::mat4 textTransform = CalculateTransform();
    textTransform = glm::translate(textTransform, {- Size().x / 2, -Size().y / 2, 0.f});
    textTransform = glm::scale(textTransform, {1.f / Scale().x, 1.f / Scale().y, 1.f});
    textTransform = glm::scale(textTransform, {10.f, 10.f, 1.f});
    // textTransform = glm::scale(textTransform, {Size().x, Size().y, 1.f});
    Renderer::get_singleton().DrawText(textTransform, m_text, Application::get_singleton().GetDefaultFont());
    NineSlicePanel::UpdateDraw();
}
FileBuffer Button::SaveImpl(object_type *out) {
	Button *o = reinterpret_cast<Button *>(out);

	YAML::Node doc = Serializer::get_singleton().SaveWithType<NineSlicePanel>(out).Yaml();
	
	return FileBuffer(doc);
}

bool Button::LoadImpl(object_type *out, const FileBuffer &buf) {
	Debug::Error("Button::LoadImpl is not implemented");
	return false;
}
} // namespace sowa
