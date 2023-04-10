#include "text2d.hpp"

#include "debug.hpp"

#include "core/application.hpp"
#include "core/renderer.hpp"
#include "resource/resource_loader.hpp"

namespace Sowa {
Text2D::Text2D() {
	_NodeType = "Text2D";
	m_Type = Typename();
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

FileBuffer Text2D::SaveImpl(ObjectType *out) {
	Text2D *o = reinterpret_cast<Text2D *>(out);

	YAML::Node doc = Serializer::get_singleton().SaveWithType<Node2D>(out).Yaml();
	doc["Text"] = o->GetText();
	
	return FileBuffer(doc);
}

bool Text2D::LoadImpl(ObjectType *out, const FileBuffer &buf) {
	Debug::Error("Text2D::LoadImpl is not implemented");
	return false;
}

} // namespace Sowa
