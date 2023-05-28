#include "nine_slice_panel.hpp"

#include "debug.hpp"

#include "core/application.hpp"
#include "core/node_db.hpp"
#include "core/renderer.hpp"
#include "gfx/graphics.hpp"
#include "resource/resource_loader.hpp"
#include "scene/scene.hpp"
#include "utils/algorithm.hpp"

#include "glm/gtc/matrix_transform.hpp"

namespace sowa {
NineSlicePanel::NineSlicePanel() {
	_NodeType = "NineSlicePanel";
	m_type = Typename();

	m_MarginLeft = 64;
	m_MarginRight = 64;
	m_MarginTop = 64;
	m_MarginBottom = 64;
}

void NineSlicePanel::Bind() {
	NodeFactory factory;
	factory.construct = []() -> Node * {
		Node *node = Allocator<NineSlicePanel>::Get().allocate(1);
		new (node) NineSlicePanel;

		return node;
	};

	factory.destruct = [](Node *node) {
		Allocator<NineSlicePanel>::Get().deallocate(reinterpret_cast<NineSlicePanel *>(node), 1);
	};

	NodeDB::Instance().RegisterNodeType("NineSlicePanel", "Node2D", factory);

	NodeDB::Instance().RegisterAttribute(
		"NineSlicePanel", "size.x", [](Node *node) -> light_variant {
		NineSlicePanel* panel = dynamic_cast<NineSlicePanel*>(node);
		if(nullptr != panel) {
			return panel->Size().x;
		}
		return 0.f; }, [](Node *node, light_variant sz) {
		NineSlicePanel* panel = dynamic_cast<NineSlicePanel*>(node);
		if(nullptr != panel) {
			panel->Size().x = sz.Float();
		} });

	NodeDB::Instance().RegisterAttribute(
		"NineSlicePanel", "size.y", [](Node *node) -> light_variant {
		NineSlicePanel* panel = dynamic_cast<NineSlicePanel*>(node);
		if(nullptr != panel) {
			return panel->Size().y;
		}
		return 0.f; }, [](Node *node, light_variant sz) {
		NineSlicePanel* panel = dynamic_cast<NineSlicePanel*>(node);
		if(nullptr != panel) {
			panel->Size().y = sz.Float();
		} });

	NodeDB::Instance().RegisterAttribute(
		"NineSlicePanel", "margin.left", [](Node *node) -> light_variant {
		NineSlicePanel* panel = dynamic_cast<NineSlicePanel*>(node);
		if(nullptr != panel) {
			return panel->MarginLeft();
		}
		return 0.f; }, [](Node *node, light_variant margin) {
		NineSlicePanel* panel = dynamic_cast<NineSlicePanel*>(node);
		if(nullptr != panel) {
			panel->MarginLeft() = margin.Float();
		} });

	NodeDB::Instance().RegisterAttribute(
		"NineSlicePanel", "margin.top", [](Node *node) -> light_variant {
		NineSlicePanel* panel = dynamic_cast<NineSlicePanel*>(node);
		if(nullptr != panel) {
			return panel->MarginTop();
		}
		return 0.f; }, [](Node *node, light_variant margin) {
		NineSlicePanel* panel = dynamic_cast<NineSlicePanel*>(node);
		if(nullptr != panel) {
			panel->MarginTop() = margin.Float();
		} });

	NodeDB::Instance().RegisterAttribute(
		"NineSlicePanel", "margin.right", [](Node *node) -> light_variant {
		NineSlicePanel* panel = dynamic_cast<NineSlicePanel*>(node);
		if(nullptr != panel) {
			return panel->MarginRight();
		}
		return 0.f; }, [](Node *node, light_variant margin) {
		NineSlicePanel* panel = dynamic_cast<NineSlicePanel*>(node);
		if(nullptr != panel) {
			panel->MarginRight() = margin.Float();
		} });

	NodeDB::Instance().RegisterAttribute(
		"NineSlicePanel", "margin.bottom", [](Node *node) -> light_variant {
		NineSlicePanel* panel = dynamic_cast<NineSlicePanel*>(node);
		if(nullptr != panel) {
			return panel->MarginBottom();
		}
		return 0.f; }, [](Node *node, light_variant margin) {
		NineSlicePanel* panel = dynamic_cast<NineSlicePanel*>(node);
		if(nullptr != panel) {
			panel->MarginBottom() = margin.Float();
		} });

	NodeDB::Instance().RegisterAttribute(
		"NineSlicePanel", "texture.path", [](Node *node) -> light_variant {
		Debug::Error("Impelement NineSlicePanel Attribute getter texture.path");
		return ""; }, [](Node *node, light_variant path) {
		NineSlicePanel* panel = dynamic_cast<NineSlicePanel*>(node);
		if(nullptr != panel) {
			panel->Texture() = ResourceLoader::get_singleton().LoadResource<ImageTexture>(path.String());
		} });
}

void NineSlicePanel::EnterScene() {
}
void NineSlicePanel::ExitScene() {
}
void NineSlicePanel::UpdateLogic() {
}
void NineSlicePanel::UpdateDraw() {
	if (m_texture != nullptr) {
		float texWidth = m_texture->Width();
		float texHeight = m_texture->Height();
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
		if (left > right) {
			Swap(left, right);
		}
		if (top > bottom) {
			Swap(top, bottom);
		}

		if (left == 1.f - right || bottom == 1.f - top) {
			left = 0.f;
			right = 0.f;
			top = 0.f;
			bottom = 0.f;
		}

		vec2 uv1 = {left, top};
		vec2 uv2 = {1.f - right, 1.f - bottom};

		static float g = 0.f;
		g += 1.f;

		glm::mat4 transform = CalculateTransform();
		Application::get_singleton().BindViewUniform(Graphics().Default2DShader(), "uView");
		Graphics().Default2DShader().Bind();
		Graphics().Default2DShader().UniformTexture("uTexture", m_texture->TextureID(), 0);
		Application::get_singleton().BindProjectionUniform(Graphics().Default2DShader(), "uProj");

		auto func = [](const glm::mat4 &transform, const vec2 &uv1, const vec2 &uv2, const vec2 &scale) {
			Graphics().Default2DShader().UniformVec4("uTexCrop", {uv1.x, uv1.y, uv2.x, uv2.y});
			Graphics().Default2DShader().UniformVec2("uBaseScale", {scale.x, scale.y});
			Graphics().Default2DShader().UniformMat4("uModel", transform);
			Graphics().DrawQuad();
		};

		// top left
		func(glm::translate(CalculateTransform(), glm::vec3{-width / 2 + m_MarginLeft / 2, height / 2 - m_MarginTop / 2, 0.f}), {0.f, 1.f - top}, {left, 1.f}, {m_MarginLeft, m_MarginBottom});
		// top edge
		func(glm::translate(CalculateTransform(), glm::vec3{0.f, height / 2 - m_MarginTop / 2, 0.f}), {left, 1.f - top}, {1.f - right, 1.f}, {width - m_MarginLeft - m_MarginRight, m_MarginBottom});
		// top right
		func(glm::translate(CalculateTransform(), glm::vec3{width / 2 - m_MarginLeft / 2, height / 2 - m_MarginTop / 2, 0.f}), {1.f - right, 1.f - top}, {1.f, 1.f}, {m_MarginRight, m_MarginBottom});
		// right edge
		func(glm::translate(CalculateTransform(), glm::vec3{width / 2 - m_MarginLeft / 2, 0.f, 0.f}), {1.f - right, bottom}, {1.f, 1.f - top}, {m_MarginRight, height - m_MarginTop - m_MarginBottom});
		// bottom right
		func(glm::translate(CalculateTransform(), glm::vec3{width / 2 - m_MarginLeft / 2, -height / 2 + m_MarginBottom / 2, 0.f}), {1.f - right, 0.f}, {1.f, bottom}, {m_MarginRight, m_MarginBottom});
		// bottom edge
		func(glm::translate(CalculateTransform(), glm::vec3{0.f, -height / 2 + m_MarginTop / 2, 0.f}), {left, 0.f}, {1.f - right, bottom}, {width - m_MarginLeft - m_MarginRight, m_MarginBottom});
		// bottom left
		func(glm::translate(CalculateTransform(), glm::vec3{-width / 2 + m_MarginLeft / 2, -height / 2 + m_MarginTop / 2, 0.f}), {0.f, 0.f}, {left, bottom}, {m_MarginLeft, m_MarginBottom});
		// left edge
		func(glm::translate(CalculateTransform(), glm::vec3{-width / 2 + m_MarginLeft / 2, 0.f, 0.f}), {0.f, bottom}, {right, 1.f - top}, {m_MarginRight, height - m_MarginTop - m_MarginBottom});
		// center
		func(glm::translate(CalculateTransform(), glm::vec3{0.f, 0.f, 0.f}), uv1, uv2, {width - m_MarginLeft - m_MarginRight, height - m_MarginTop - m_MarginBottom});

		Graphics().Default2DShader().UniformVec4("uTexCrop", {0.f, 0.f, 1.f, 1.f});
		Graphics().Default2DShader().UniformVec2("uBaseScale", {1.f, 1.f});
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
