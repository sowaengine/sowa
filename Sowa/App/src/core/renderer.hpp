#ifndef _E_RENDERER_HPP__
#define _E_RENDERER_HPP__

#pragma once

#include "resource/font/font.hpp"
#include "resource/texture/image_texture.hpp"
#include <glm/glm.hpp>

#include "math/math.hpp"

#include "stlpch.hpp"

namespace sowa {

class Renderer {
  public:
	static Renderer &get_singleton() {
		static Renderer renderer;
		return renderer;
	}

	void DrawTexture(const glm::vec2 &position, const glm::vec2 &scale, float zIndex, float rotation, sowa::ImageTexture &texture, uint32_t id = 0);
	void DrawTexture(const glm::mat4 &transform, sowa::ImageTexture &texture, const vec2& scale = vec2{1.f, 1.f}, uint32_t id = 0);
	void DrawTextureWithUV(const glm::mat4 &transform, sowa::ImageTexture &texture, const vec2& uv1, const vec2& uv2, const vec2& scale = vec2{1.f, 1.f}, uint32_t id = 0);
	void DrawText(const glm::vec2& position, float scale, float rotation, const std::string& text, sowa::Font& font);
	void DrawText(const glm::mat4& transform, const std::string& text, sowa::Font& font);

	void DrawLine(const glm::vec2 &p1, const glm::vec2 &p2, float thickness, glm::vec4 color = {1.f, 1.f, 1.f, 1.f});
	void DrawLine(const glm::vec2 &p1, const glm::vec2 &p2, float thickness, glm::vec3 color = {1.f, 1.f, 1.f});

  private:
	friend class Application;
  // Called from Application
	bool LoadFont(sowa::Font& font, const unsigned char* data, unsigned size);

	Renderer();
	~Renderer();
};

} // namespace sowa
#endif