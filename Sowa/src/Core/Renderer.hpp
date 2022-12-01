#ifndef _E_RENDERER_HPP__
#define _E_RENDERER_HPP__

#pragma once

#include "ECS/Components/NinePatchRect/NinePatchRect.hpp"
#include "Resource/Texture/Texture.hpp"
#include <glm/glm.hpp>
#include <string>

namespace Sowa {

class Renderer {
  public:
	static Renderer &get_singleton() {
		static Renderer renderer;
		return renderer;
	}

	void DrawTexture(const glm::vec2 &position, const glm::vec2 &scale, float zIndex, float rotation, Sowa::ImageTexture &texture, uint32_t id = 0);

	void DrawLine(const glm::vec2 &p1, const glm::vec2 &p2, float thickness, glm::vec3 color = {1.f, 1.f, 1.f});

  private:
	Renderer();
	~Renderer();
};

} // namespace Sowa
#endif