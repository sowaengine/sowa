#ifndef _E_RENDERER_HPP__
#define _E_RENDERER_HPP__

#pragma once

#include "resource/texture/texture.hpp"
#include "resource/font/font.hpp"
#include <glm/glm.hpp>

#include "utils/math.hpp"

#include "stlpch.hpp"

namespace Sowa {

class Renderer {
  public:
	static Renderer &get_singleton() {
		static Renderer renderer;
		return renderer;
	}

	void DrawTexture(const glm::vec2 &position, const glm::vec2 &scale, float zIndex, float rotation, Sowa::ImageTexture &texture, uint32_t id = 0);
	void DrawTexture(const glm::mat4 &transform, Sowa::ImageTexture &texture, const Vector2& scale = Vector2{1.f, 1.f}, uint32_t id = 0);
	void DrawText(const glm::vec2& position, float scale, float rotation, const std::string& text, Sowa::Font& font);
	void DrawText(const glm::mat4& transform, const std::string& text, Sowa::Font& font);

	void DrawLine(const glm::vec2 &p1, const glm::vec2 &p2, float thickness, glm::vec3 color = {1.f, 1.f, 1.f});

  private:
	friend class Application;
  // Called from Application
	bool LoadFont(Sowa::Font& font, unsigned char* data, unsigned size);

	Renderer();
	~Renderer();
};

} // namespace Sowa
#endif