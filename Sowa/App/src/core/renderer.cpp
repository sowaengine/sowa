#include "core/renderer.hpp"
#include "resource/texture/texture.hpp"

#include "core/application.hpp"

#include "Core/nm_Matrix.hpp"
#include "nmGfx/src/Core/nm_Renderer.hpp"

#include "stlpch.hpp"

#include "res/shaders/text.glsl.res.hpp"

namespace Sowa {
Renderer::Renderer() {
}

Renderer::~Renderer() {
}

void Renderer::DrawTexture(const glm::vec2 &position, const glm::vec2 &scale, float zIndex, float rotation, Sowa::ImageTexture &texture, uint32_t id /*= 0*/) {
	Application::get_singleton()._renderer->DrawTexture(&texture._texture, nmGfx::CalculateModelMatrix({position.x, -position.y, zIndex}, {0.f, 0.f, rotation}, {scale.x, scale.y, 1.f}), {1.f, 1.f, 1.f}, id);
}

void Renderer::DrawText(const glm::vec2& position, float scale, float rotation, const std::string& text, Sowa::Font& font) {
	static nmGfx::Shader shader;
	static bool loaded = false;
	if(!loaded) {
		shader.LoadText(std::string(reinterpret_cast<char *>(Res::App_include_res_shaders_text_glsl_data.data()), Res::App_include_res_shaders_text_glsl_data.size()));
		
		glm::mat4 projection = nmGfx::CalculateProjectionMatrix(1920, 1080, .5f, .5f, 0.f, 10.f);
		shader.UniformMat4("projection", projection);
		shader.UniformVec3("textColor", {1.f, 1.f, 1.f});

		loaded = true;
	}
	
	shader.UniformMat4("model", nmGfx::CalculateModelMatrix({position.x, -position.y}, rotation, {scale, scale}, {0.f, 0.f}));
	Application::get_singleton()._renderer->DrawText(shader, font._Font, text, scale);
}

void Renderer::DrawLine(const glm::vec2 &p1, const glm::vec2 &p2, float thickness, glm::vec3 color) {
	static nmGfx::Texture texture;
	static bool loaded = false;
	if (!loaded) {
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