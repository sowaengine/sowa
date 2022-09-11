#ifndef _E_RENDERER_HPP__
#define _E_RENDERER_HPP__

#pragma once

#include "ECS/Components/NinePatchRect/NinePatchRect.hpp"
#include "Resource/Texture/Texture.hpp"
#include <glm/glm.hpp>
#include <string>

namespace Ease {

class Renderer {
  public:
	static Renderer &get_singleton() {
		static Renderer renderer;
		return renderer;
	}

	void DrawTexture(const glm::vec2 &position, const glm::vec2 &scale, float zIndex, float rotation, Ease::Texture &texture, uint32_t id = 0);
	// void DrawQuad(const glm::vec2& position, const glm::vec2& scale, float zIndex, float rotation, Texture& texture, const glm::vec2& uv1, const glm::vec2& uv2);
	// void DrawQuadWithID(const glm::vec2& position, const glm::vec2& scale, float zIndex, float rotation, Texture& texture, uint32_t id, const glm::vec2& uv1, const glm::vec2& uv2);
	// void DrawQuadColor(const glm::vec2& center, const glm::vec2& size, float rotation, Color color, const glm::vec2& offset = glm::vec2{0.f, 0.f});
	// void DrawCircleColor(const glm::vec2& center, float radius, float rotation, Color color, const glm::vec2& offset = glm::vec2{0.f, 0.f});
	// void DrawText(const glm::vec2& position, const glm::vec2& scale, float zIndex, float rotation, float fontSize, const std::string& text, const glm::vec4& color);
	// void DrawTextWithID(const glm::vec2& position, const glm::vec2& scale, float zIndex, float rotation, float fontSize, const std::string& text, const glm::vec4& color, uint32_t id);
	// void DrawTextAligned(const glm::vec2& position, const glm::vec2& size, const glm::vec2& alignment, float zIndex, float rotation, float fontSize, const std::string& text, const glm::vec4& color);
	// void DrawNinePatchRect(const glm::vec2& position, const glm::vec2& size, const glm::vec2& scale, float zIndex, float rotation, Ease::Component::NinePatchRect& npatch);
	// void DrawNinePatchRectWithID(const glm::vec2& position, const glm::vec2& size, const glm::vec2& scale, float zIndex, float rotation, Ease::Component::NinePatchRect& npatch, uint32_t id);
  private:
	Renderer();
	~Renderer();

	// Shader m_PickingShader;
};

} // namespace Ease
#endif