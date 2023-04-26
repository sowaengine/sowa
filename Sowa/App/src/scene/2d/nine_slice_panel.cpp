#include "nine_slice_panel.hpp"

#include "debug.hpp"

#include "core/application.hpp"
#include "core/renderer.hpp"
#include "resource/resource_loader.hpp"
#include "scene/scene.hpp"
#include "utils/algorithm.hpp"

#include "glm/gtc/matrix_transform.hpp"

namespace sowa {
NineSlicePanel::NineSlicePanel() {
	_NodeType = "NineSlicePanel";
	m_type = Typename();

	m_MarginLeft = 256;
	m_MarginRight = 256;
	m_MarginTop = 256;
	m_MarginBottom = 256;
}

void NineSlicePanel::EnterScene() {
}
void NineSlicePanel::ExitScene() {
}
void NineSlicePanel::UpdateLogic() {
}
void NineSlicePanel::UpdateDraw() {
	if (_Texture != nullptr) {
		float texWidth = _Texture->Width();
		float texHeight = _Texture->Height();
		float width = m_Size.x;
		float height = m_Size.y;

		float left = map(m_MarginLeft, 0.f, texWidth, 0.f, 1.f);
		float right = map(m_MarginRight, 0.f, texWidth, 0.f, 1.f);
		float top = map(m_MarginTop, 0.f, texHeight, 0.f, 1.f);
		float bottom = map(m_MarginBottom, 0.f, texHeight, 0.f, 1.f);
		
		left = minmax(left, 0.f, 1.f);
		right = minmax(right, 0.f, 1.f);
		top = minmax(top, 0.f, 1.f);
		bottom = minmax(bottom, 0.f, 1.f);
		if(left > right) {
			Swap(left, right);
		}
		if(top > bottom) {
			Swap(top, bottom);
		}
		
		if(left == 1.f - right || bottom == 1.f - top) {
			left = 0.f;
			right = 0.f;
			top = 0.f;
			bottom = 0.f;
		}

		vec2 uv1 = {left, top};
		vec2 uv2 = {1.f - right, 1.f - bottom};

		static float g = 0.f;
		g += 1.f;
		// top left
		Renderer::get_singleton().DrawTextureWithUV(glm::translate(CalculateTransform(), glm::vec3{-width / 2 + m_MarginLeft / 2, height / 2 - m_MarginTop / 2, 0.f}), *_Texture.get(), {0.f, 1.f - top}, {left, 1.f}, {m_MarginLeft, m_MarginBottom}, ID());
		// top edge
		Renderer::get_singleton().DrawTextureWithUV(glm::translate(CalculateTransform(), glm::vec3{0.f, height / 2 - m_MarginTop / 2, 0.f}), *_Texture.get(), {left, 1.f - top}, {1.f - right, 1.f}, {width - m_MarginLeft - m_MarginRight, m_MarginBottom}, ID());
		// top right
		Renderer::get_singleton().DrawTextureWithUV(glm::translate(CalculateTransform(), glm::vec3{width / 2 - m_MarginLeft / 2, height / 2 - m_MarginTop / 2, 0.f}), *_Texture.get(), {1.f - right, 1.f - top}, {1.f, 1.f}, {m_MarginRight, m_MarginBottom}, ID());
		// right edge
		Renderer::get_singleton().DrawTextureWithUV(glm::translate(CalculateTransform(), glm::vec3{width / 2 - m_MarginLeft / 2, 0.f, 0.f}), *_Texture.get(), {1.f - right, bottom}, {1.f, 1.f - top}, {m_MarginRight, height - m_MarginTop - m_MarginBottom}, ID());
		// bottom right
		Renderer::get_singleton().DrawTextureWithUV(glm::translate(CalculateTransform(), glm::vec3{width / 2 - m_MarginLeft / 2, -height / 2 + m_MarginBottom / 2, 0.f}), *_Texture.get(), {1.f - right, 0.f}, {1.f, bottom}, {m_MarginRight, m_MarginBottom}, ID());
		// bottom edge
		Renderer::get_singleton().DrawTextureWithUV(glm::translate(CalculateTransform(), glm::vec3{0.f, -height / 2 + m_MarginTop / 2, 0.f}), *_Texture.get(), {left, 0.f}, {1.f - right, bottom}, {width - m_MarginLeft - m_MarginRight, m_MarginBottom}, ID());
		// bottom left
		Renderer::get_singleton().DrawTextureWithUV(glm::translate(CalculateTransform(), glm::vec3{-width / 2 + m_MarginLeft / 2, -height / 2 + m_MarginTop / 2, 0.f}), *_Texture.get(), {0.f, 0.f}, {left, bottom}, {m_MarginLeft, m_MarginBottom}, ID());
		// left edge
		Renderer::get_singleton().DrawTextureWithUV(glm::translate(CalculateTransform(), glm::vec3{-width / 2 + m_MarginLeft / 2, 0.f, 0.f}), *_Texture.get(), {0.f, bottom}, {right, 1.f - top}, {m_MarginRight, height - m_MarginTop - m_MarginBottom}, ID());
		// center
		Renderer::get_singleton().DrawTextureWithUV(glm::translate(CalculateTransform(), glm::vec3{0.f, 0.f, 0.f}), *_Texture.get(), uv1, uv2, {width - m_MarginLeft - m_MarginRight, height - m_MarginTop - m_MarginBottom}, ID());
	}
}
FileBuffer NineSlicePanel::SaveImpl(object_type *out) {
	NineSlicePanel *o = reinterpret_cast<NineSlicePanel *>(out);

	YAML::Node doc = Serializer::get_singleton().SaveWithType<Node2D>(out).Yaml();
	
	return FileBuffer(doc);
}

bool NineSlicePanel::LoadImpl(object_type *out, const FileBuffer &buf) {
	Debug::Error("NineSlicePanel::LoadImpl is not implemented");
	return false;
}
} // namespace sowa
