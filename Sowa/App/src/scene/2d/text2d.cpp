#include "text2d.hpp"

#include "debug.hpp"

#include "core/application.hpp"
#include "core/renderer.hpp"
#include "resource/resource_loader.hpp"

#include "gfx/graphics.hpp"

namespace sowa {
Text2D::Text2D() {
	_NodeType = "Text2D";
	m_type = Typename();
}

void Text2D::EnterScene() {
}
void Text2D::ExitScene() {
}
void Text2D::UpdateLogic() {
}
void Text2D::UpdateDraw() {
	gfx::IFont* fontHandle = m_font != nullptr ? m_font.get() : Application::get_singleton().GetDefaultFont();

	{
		gfx::DrawTextUIArgs args;
		args.drawMode = gfx::TextDrawMode::WordWrap;
		args.targetWidth = -1.f;
		args.transform = CalculateTransform();

		Graphics().DrawTextUI(m_text, fontHandle, args);
	}
}

FileBuffer Text2D::SaveImpl(object_type *out) {
	Text2D *o = reinterpret_cast<Text2D *>(out);

	YAML::Node doc = Serializer::get_singleton().SaveWithType<Node2D>(out).Yaml();
	doc["Text"] = o->GetText();
	
	return FileBuffer(doc);
}

bool Text2D::LoadImpl(object_type *out, const FileBuffer &buf) {
	Debug::Error("Text2D::LoadImpl is not implemented");
	return false;
}

} // namespace sowa
