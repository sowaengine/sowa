#include "Core/Renderer.hpp"
#include "Resource/Texture/Texture.hpp"

#include "Core/Application.hpp"
#include "Core/nm_Matrix.hpp"
#include "nmGfx/src/Core/nm_Renderer.hpp"

#include "stlpch.hpp"

namespace Sowa {
Renderer::Renderer() {
}

Renderer::~Renderer() {
}

void Renderer::DrawTexture(const glm::vec2 &position, const glm::vec2 &scale, float zIndex, float rotation, Sowa::ImageTexture &texture, uint32_t id /*= 0*/) {
	Application::get_singleton()._renderer->DrawTexture(&texture._texture, nmGfx::CalculateModelMatrix({position.x, -position.y, zIndex}, {0.f, 0.f, rotation}, {scale.x, scale.y, 1.f}), {1.f, 1.f, 1.f}, id);
}

void Renderer::DrawLine(const glm::vec2 &p1, const glm::vec2 &p2, float thickness, glm::vec3 color) {
	static nmGfx::Texture texture;
	static bool loaded = false;
	if(!loaded) {
		unsigned char texture_data[] = {255, 255, 255};
		texture.LoadFromData(texture_data, 1, 1, 3, nmGfx::TextureType::TEXTURE2D);

		loaded = true;
	}

	float rot = atan2(p1.y - p2.y, p1.x - p2.x) * 180 / PI;
	
	glm::vec2 sub = {p2.x - p1.x, p2.y - p1.y};
	float len = sqrt((sub.x * sub.x) + (sub.y * sub.y));

	auto mat = nmGfx::CalculateModelMatrix({p1.x, p1.y}, rot, {len, thickness}, {-len / 2.f, 0.f});

	Application::get_singleton()._renderer->DrawTexture(&texture, mat, color);
}

} // namespace Sowa